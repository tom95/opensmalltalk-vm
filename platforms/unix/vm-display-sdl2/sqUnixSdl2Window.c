/* sqUnixSdl2Window.c -- support for display via your sdl2 window system.
 * 
 * Last edited: 2006-04-17 16:57:12 by piumarta on margaux.local
 * 
 * This is a template for creating your own window drivers for Squeak:
 * 
 *   - copy the entire contents of this directory to some other name
 *   - rename this file to be something more appropriate
 *   - modify acinclude.m4, Makefile.in, and ../vm/sqUnixMain accordingly
 *   - implement all the stubs in this file that currently do nothing
 * 
 */

#include "sq.h"
#include "sqMemoryAccess.h"
#include "sqaio.h"

#include "sqUnixMain.h"
#include "sqUnixGlobals.h"
#include "sqUnixCharConv.h"		/* not required, but probably useful */
#include "aio.h"			/* ditto */

#include "SqDisplay.h"

#include <stdio.h>
#include <SDL2/SDL.h>

#include "sqUnixEvent.c"		/* see X11 and/or Quartz drivers for examples */

#include "common.h"

SDL_Window *window;
ApplicationState state;

#define trace() fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__)

int sdl_button_to_sq(int button) {
  switch (button) {
    case SDL_BUTTON_LEFT:
      return RedButtonBit;
    case SDL_BUTTON_RIGHT:
      return BlueButtonBit;
    case SDL_BUTTON_MIDDLE:
      return YellowButtonBit;
    case SDL_BUTTON_X1:
    case SDL_BUTTON_X2:
      return 0;
  }
  return 0;
}

int sdl_modifiers_to_sq(SDL_Keymod mod) {
  return
    (mod & KMOD_SHIFT ? ShiftKeyBit : 0) |
    (mod & KMOD_CTRL ? CtrlKeyBit : 0) |
    (mod & KMOD_ALT ? CommandKeyBit : 0) |
    (mod & KMOD_GUI ? OptionKeyBit : 0);
}

int current_modifiers() {
    return sdl_modifiers_to_sq(SDL_GetModState());
}

SDL_Surface *surface_from_sq_bits(sqInt bitsIndex, sqInt width, sqInt height, sqInt depth)
{
  char *bits = pointerForOop(bitsIndex);

  Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000;
  gmask = 0x000000ff;
  bmask = 0x0000ff00;
  amask = 0x00ff0000;
#else
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
  amask = 0xff000000;
#endif

  return SDL_CreateRGBSurfaceFrom(bits, width, height, depth, depth / 8 * width, rmask, gmask, bmask, amask);
}

#if 1
// TODO: found on the internet; replace this
int get_length(unsigned char ch) {
  int l;
  unsigned char c = ch;
  c >>= 3;
  // 6 => 0x7e
  // 5 => 0x3e
  if (c == 0x1e) { l = 4; }
  else {
    c >>= 1;
    if (c == 0xe) { l = 3; }
    else {
      c >>= 1;
      if (c == 0x6) { l = 2; }
      else { l = 1; }
    }
  }
  return l;
}
unsigned long *to_unicode(unsigned char *utf8, int len) {
  unsigned char *p = utf8;
  unsigned long ch;
  int x = 0;
  int l;
  unsigned long *result = (unsigned long *)malloc(sizeof(unsigned long)*len);
  unsigned long *r = result;
  if (!result) { return NULL; }
  while (*p) {
    l = get_length(*p);
    switch (l) {
      case 4:
	ch = (*p ^ 0xf0);
	break;
      case 3:
	ch = (*p ^ 0xe0);
	break;
      case 2:
	ch = (*p ^ 0xc0);
	break;
      case 1:
	ch = *p;
	break;
      default:
	printf("Len: %i\n", l);
    }
    ++p;
    int y;
    for (y = l; y > 1; y--) {
      ch <<= 6;
      ch |= (*p ^ 0x80);
      ++p;
    }
    x += l;
    *r = ch;
    r++;
  }
  *r = 0x0;
  return result;
}
#endif

int translateCodeMaybe(SDL_Keycode code) {
  switch (code) {
    case SDLK_HOME:	return  1;
    case SDLK_END:	return  4;
    case SDLK_INSERT:	return  5;
    case SDLK_BACKSPACE:return  8;
    case SDLK_TAB:	return  9;
    case SDLK_PAGEUP:	return 11;
    case SDLK_PAGEDOWN:	return 12;
    case SDLK_RETURN:	return 13;
    case SDLK_ESCAPE:	return 27;
    case SDLK_LEFT:	return 28;
    case SDLK_RIGHT:	return 29;
    case SDLK_UP:	return 30;
    case SDLK_DOWN:	return 31;
    case SDLK_PRIOR:
    case SDLK_DELETE:	return 127;
    default: 		return -1;
  }
}

static void handleEvent(SDL_Event *event)
{
  unsigned int sym;
  int was_translated;
  int ucs4;
  int modifiers;
  int keyCode;
  char charCode;
  char name;
  switch (event->type) {
    case SDL_WINDOWEVENT:
      switch (event->window.event) {
	case SDL_WINDOWEVENT_EXPOSED:
	case SDL_WINDOWEVENT_SHOWN:
	  fullDisplayUpdate();
	  break;
	case SDL_WINDOWEVENT_CLOSE:
	  recordWindowEvent(WindowEventClose, 0, 0, 0, 0, 1); /* windowIndex 1 is main window */
	  break;
	case SDL_WINDOWEVENT_RESIZED:
	  destroy_context(&state);
	  init_context(&state);
	  break;
      }
      break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
      modifiers = current_modifiers();

      keyCode = translateCodeMaybe(event->key.keysym.sym);
      recordKeyboardEvent(0,
	  event->type == SDL_KEYDOWN ? EventKeyDown : EventKeyUp,
	  modifiers,
	  ucs4);

      // control characters
      if (keyCode >= 0 && event->type == SDL_KEYDOWN) {
	recordKeyboardEvent(keyCode, EventKeyChar, current_modifiers(), 0);
      }
      break;

    case SDL_TEXTINPUT:
    {
      unsigned long *res = to_unicode(event->text.text, 2);
      recordKeyboardEvent(res[0], EventKeyChar, current_modifiers(), res[0]);
      break;
    }

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      modifierState = current_modifiers();
      mousePosition.x = event->button.x;
      mousePosition.y = event->button.y;
      if (event->type == SDL_MOUSEBUTTONDOWN)
	buttonState |= sdl_button_to_sq(event->button.button);
      else
	buttonState &= ~sdl_button_to_sq(event->button.button);
      recordMouseEvent();
      break;

    case SDL_MOUSEWHEEL:
      recordMouseWheelEvent(event->wheel.x * 120, event->wheel.y * 120, current_modifiers());
      break;

    case SDL_MOUSEMOTION:
      modifierState = current_modifiers();
      mousePosition.x = event->motion.x;
      mousePosition.y = event->motion.y;
      recordMouseEvent();
      break;

    case SDL_DROPFILE:
      if (uxDropFileCount > 0) {
	SDL_free(uxDropFileNames[0]);
      }

      uxDropFileCount = 1;
      if (!uxDropFileNames)
	uxDropFileNames = SDL_malloc(sizeof(char *));
      uxDropFileNames[0] = event->drop.file;
      recordDragEvent(SQDragDrop, 1);
      break;
    /* only from sdl 2.0.5 onwards
    case SDL_DROPTEXT:
      printf("Drop text\n");
      SDL_Free(event->drop.file);
      break;
    case SDL_DROPBEGIN:
      printf("Drop begin\n");
      break;
    case SDL_DROPCOMPLETE:
      printf("Drop begin\n");
      break;*/
  }
}

static int handleEvents(void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    handleEvent(&event);
  }

  return 1;	/* 1 if events processed */
}

static void xHandler(int fd, void *data, int flags)
{
  handleEvents();
  aioHandle(1, xHandler, AIO_RX);
}

static sqInt display_dndOutStart(char *types, int ntypes)	{ return 0; }
static void  display_dndOutSend (char *bytes, int nbytes)	{ return  ; }
static sqInt display_dndOutAcceptedType(char *buf, int nbuf)	{ return 0; }
static sqInt display_dndReceived(char *fileName)	{ return 0; }
/*static sqInt display_ioGetButtonState(void)		{ return 0; }
static sqInt display_ioPeekKeystroke(void)		{ return 0; }
static sqInt display_ioGetKeystroke(void)		{ return 0; }
static sqInt display_ioGetNextEvent(sqInputEvent *evt)	{ return 0; }
static sqInt display_ioMousePoint(void)			{ return 0; }*/

static sqInt display_clipboardSize(void)
{
  trace();
  return 0;
}

static sqInt display_clipboardWriteFromAt(sqInt count, sqInt byteArrayIndex, sqInt startIndex)
{
  trace();
  return 0;
}

static sqInt display_clipboardReadIntoAt(sqInt count, sqInt byteArrayIndex, sqInt startIndex)
{
  trace();
  return 0;
}

static char **display_clipboardGetTypeNames(void)
{
  return 0;
}

static sqInt display_clipboardSizeWithType(char *typeName, int nTypeName)
{
  return 0;
}

static void display_clipboardWriteWithType(char *data, size_t ndata, char *typeName, size_t nTypeName, int isDnd, int isClaiming)
{
  return;
}

static sqInt display_ioFormPrint(sqInt bitsIndex, sqInt width, sqInt height, sqInt depth, double hScale, double vScale, sqInt landscapeFlag)
{
  trace();
  return false;
}

static sqInt display_ioBeep(void)
{
  trace();
  return 0;
}

static sqInt display_ioRelinquishProcessorForMicroseconds(sqInt microSeconds)
{
  aioSleepForUsecs(handleEvents() ? 0 : microSeconds);
  return 0;
}

static sqInt display_ioProcessEvents(void)
{
  handleEvents();
  aioPoll(0);
  return 0;
}

static double display_ioScreenScaleFactor(void)
{
  trace();
  float dpi;
  if (SDL_GetDisplayDPI(0 /*FIXME display index*/, NULL, &dpi, NULL))
    return dpi / 96.0;

  return 1.0;
}

static sqInt display_ioScreenDepth(void)
{
  return 32;
}

static sqInt display_ioScreenSize(void)
{
  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  return (width << 16) | height;
}

static unsigned char swapBits(unsigned char in)
{
  unsigned char out= 0;
  int i;
  for (i= 0; i < 8; i++)
    {
      out= (out << 1) + (in & 1);
      in >>= 1;
    }
  return out;
}

static sqInt display_ioSetCursorWithMask(sqInt cursorBitsIndex, sqInt cursorMaskIndex, sqInt offsetX, sqInt offsetY)
{
  unsigned int *cursorBits = (unsigned char *) pointerForOop(cursorBitsIndex);
  unsigned int *cursorMask = (unsigned char *) pointerForOop(cursorMaskIndex);

  int i;
  unsigned char data[32], mask[32];	/* cursors are always 16x16 */
  for (i= 0; i < 16; i++)
  {
    data[i*2+0]= (cursorBits[i] >> 24) & 0xFF;
    data[i*2+1]= (cursorBits[i] >> 16) & 0xFF;
    mask[i*2+0]= (cursorMask[i] >> 24) & 0xFF;
    mask[i*2+1]= (cursorMask[i] >> 16) & 0xFF;
  }

  static SDL_Cursor *cursor;
  if (cursor)
    SDL_FreeCursor(cursor);

  cursor = SDL_CreateCursor(data, mask, 16, 16, -offsetX, -offsetY);
  if (!cursor)
    printf("Creating cursor failed: %s\n", SDL_GetError());
  SDL_SetCursor(cursor);
  return 0;
}

static sqInt display_ioSetCursorARGB(sqInt cursorBitsIndex, sqInt extentX, sqInt extentY, sqInt offsetX, sqInt offsetY)
{
  trace();
  static SDL_Surface *cursor_surface;
  static SDL_Cursor *cursor;

  if (cursor)
    SDL_FreeCursor(cursor);
  if (cursor_surface)
    SDL_FreeSurface(cursor_surface);

  cursor_surface = surface_from_sq_bits(cursorBitsIndex, extentX, extentY, 32);
  cursor = SDL_CreateColorCursor(cursor_surface, -offsetX, -offsetY);
  SDL_SetCursor(cursor);
  return 0;
}

static sqInt display_ioSetFullScreen(sqInt fullScreen)
{
  return SDL_SetWindowFullscreen(window, fullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

static sqInt display_ioForceDisplayUpdate(void)
{
  // trace();
  // SDL_UpdateWindowSurface(window);

  /*glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  draw_window(&state);
  SDL_GL_SwapWindow(window);*/
  return 0;
}

static void draw_test() {
  int dw, dh;
  SDL_GL_GetDrawableSize(window, &dw, &dh);

  glViewport(0, 0, dw, dh);
  glClearColor(1, 1, 1, 1);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

}

static sqInt display_ioShowDisplay(sqInt dispBitsIndex, sqInt width, sqInt height, sqInt depth,
				   sqInt affectedL, sqInt affectedR, sqInt affectedT, sqInt affectedB)
{
  if (width < 1 || height < 1)
    return 0;

  if (affectedL > width)  affectedL= width;
  if (affectedR > width)  affectedR= width;
  if (affectedT > height) affectedT= height;
  if (affectedB > height) affectedB= height;

  // SDL_Rect rect;
  // rect.x = affectedL;
  // rect.y = affectedT;
  // rect.w = affectedR - affectedL;
  // rect.h = affectedB - affectedT;

  // SDL_Surface *screen = SDL_GetWindowSurface(window);
  // printf("b %i\n", screen->locked);

  // TODO optimize: can we create a smaller surface?
  // SDL_Surface *image = surface_from_sq_bits(dispBitsIndex, width, height, depth);
  // SDL_BlitSurface(image, &rect, screen, &rect);
  // SDL_UpdateWindowSurface(window);

  // SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, image);
  // SDL_FreeSurface(image);

  blit(&state, pointerForOop(dispBitsIndex), width, height, depth,
      affectedL, affectedR, affectedT, affectedB);

  // draw_text(&state, 200, 200);
  SDL_GL_SwapWindow(state.window);

  return 0;
}

static void *display_ioSkiaContext() {
  return state.canvas;
}

static sqInt display_ioHasDisplayDepth(sqInt i)
{
  return 32 == i;
}

static sqInt display_ioSetDisplayMode(sqInt width, sqInt height, sqInt depth, sqInt fullscreenFlag)
{
  trace();
  setSavedWindowSize((width << 16) + (height & 0xFFFF));
  setFullScreenFlag(fullscreenFlag);
  return 0;
}

static void display_winSetName(char *imageName)
{
  SDL_SetWindowTitle(window, imageName);
}

static void *display_ioGetDisplay(void)	{ return 0; }
static void *display_ioGetWindow(void)	{ return 0; }

static sqInt display_ioGLinitialise(void) { trace();  return 0; }
static sqInt display_ioGLcreateRenderer(glRenderer *r, sqInt x, sqInt y, sqInt w, sqInt h, sqInt flags) { trace();  return 0; }
static void  display_ioGLdestroyRenderer(glRenderer *r) { trace(); }
static void  display_ioGLswapBuffers(glRenderer *r) { trace(); }
static sqInt display_ioGLmakeCurrentRenderer(glRenderer *r) { trace();  return 0; }
static void  display_ioGLsetBufferRect(glRenderer *r, sqInt x, sqInt y, sqInt w, sqInt h) { trace(); }

static char *display_winSystemName(void)
{
  return "SDL2";
}

#define handle_sdl_error(msg)

static void display_winInit(void)
{
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  static const int kStencilBits = 8;  // Skia needs 8 stencil bits
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, kStencilBits);

  SDL_GL_SetSwapInterval(1);

  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    handle_sdl_error("Could not init sdl");
  }
}

void get_window_size(int *width, int *height)
{
  int s = getSavedWindowSize();
  if (s) {
    *width = s >> 16;
    *height = s & 0xffff;
  } else {
    *width = 640;
    *height = 480;
  }
}

static void display_winOpen(int argc, char *dropFiles[])
{
  int width, height;
  get_window_size(&width, &height);

  window = SDL_CreateWindow(
      "Squeak SDL2",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_OPENGL /*| SDL_WINDOW_ALLOW_HIGHDPI*/ | SDL_WINDOW_RESIZABLE);

  SDL_StartTextInput();

  if (!window) {
    handle_sdl_error("Could not create window");
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    handle_sdl_error("Could not create gl contexts");
  }
  if (SDL_GL_MakeCurrent(window, gl_context)) {
    handle_sdl_error("could not activate gl context")
  }

  state.window = window;
  init_context(&state);

  aioEnable(1, 0, AIO_EXT);
  aioHandle(1, xHandler, AIO_RX);

  (void) recordKeystroke;
  (void) recordDragEvent;
}

static void display_winExit(void)
{
  aioDisable(1);
  SDL_StopTextInput();
  SDL_DestroyWindow(window);
  SDL_Quit();
}

static long  display_winImageFind(char *buf, int len)		{ trace();  return 0; }
static void display_winImageNotFound(void)			{ trace(); }

/*
 * Browser Plugin
 */
static sqInt display_primitivePluginBrowserReady(void)		{ return primitiveFail(); }
static sqInt display_primitivePluginRequestURLStream(void)	{ return primitiveFail(); }
static sqInt display_primitivePluginRequestURL(void)		{ return primitiveFail(); }
static sqInt display_primitivePluginPostURL(void)		{ return primitiveFail(); }
static sqInt display_primitivePluginRequestFileHandle(void)	{ return primitiveFail(); }
static sqInt display_primitivePluginDestroyRequest(void)	{ return primitiveFail(); }
static sqInt display_primitivePluginRequestState(void)		{ return primitiveFail(); }

/*
 * Host window
 */
#if (SqDisplayVersionMajor >= 1 && SqDisplayVersionMinor >= 2)
static long display_hostWindowClose(long index)             { return 0; }
static long display_hostWindowCreate(long w, long h, long x, long y,
  char *list, long attributeListLength)                     { return 0; }
static long display_hostWindowShowDisplay(unsigned *dispBitsIndex, long width, long height, long depth,
  long affectedL, long affectedR, long affectedT, long affectedB, long windowIndex)              { return 0; }
static long display_hostWindowGetSize(long windowIndex)     { return -1; }
static long display_hostWindowSetSize(long windowIndex, long w, long h) { return -1; }
static long display_hostWindowGetPosition(long windowIndex) { return -1; }
static long display_hostWindowSetPosition(long windowIndex, long x, long y) { return -1; }
static long display_hostWindowSetTitle(long windowIndex, char *newTitle, long sizeOfTitle)     { return -1; }
static long display_hostWindowCloseAll(void)                { return 0; }
#endif

#if (SqDisplayVersionMajor >= 1 && SqDisplayVersionMinor >= 3)
long display_ioSetCursorPositionXY(long x, long y) { return 0; }
long display_ioPositionOfScreenWorkArea (long windowIndex) { return -1; }
long display_ioSizeOfScreenWorkArea (long windowIndex) { return -1; }
void *display_ioGetWindowHandle() { return 0; }
long display_ioPositionOfNativeDisplay(void *windowHandle) { return -1; }
long display_ioSizeOfNativeDisplay(void *windowHandle) { return -1; }
long display_ioPositionOfNativeWindow(void *windowHandle) { return -1; }
long display_ioSizeOfNativeWindow(void *windowHandle) { return -1; }
#endif /* (SqDisplayVersionMajor >= 1 && SqDisplayVersionMinor >= 3) */

SqDisplayDefine(sdl2);	/* name must match that in makeInterface() below */


/*** module ***/


static void display_printUsage(void)
{
  printf("\nCustom Window <option>s: (none)\n");
  /* otherwise... */
}

static void display_printUsageNotes(void)
{
  trace();
}

static void display_parseEnvironment(void)
{
  trace();
}

static int display_parseArgument(int argc, char **argv)
{
  return 0;	/* arg not recognised */
}

static void *display_makeInterface(void)
{
  return &display_sdl2_itf;		/* name must match that in SqDisplayDefine() above */
}

#include "SqModule.h"

SqModuleDefine(display, sdl2);	/* name must match that in sqUnixMain.c's moduleDescriptions */
