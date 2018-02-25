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

SDL_Window *window;


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
    (ShiftKeyBit & (mod & KMOD_SHIFT)) |
    (CtrlKeyBit & (mod & KMOD_CTRL)) |
    (CommandKeyBit & (mod & KMOD_GUI)) |
    (OptionKeyBit & (mod & KMOD_ALT));
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

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1
static const uint8_t utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};
uint32_t utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte)
{
  uint32_t type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state*16 + type];
  return *state;
}
// end copyright

static void handleEvent(SDL_Event *event)
{
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
      }
      break;

    case SDL_KEYDOWN:
      printf("DOWN %d\n", event->key.keysym.sym);
      recordKeyboardEvent(event->key.keysym.sym, EventKeyDown, current_modifiers(), 0);
      // recordKeyboardEvent(event->key.keysym.sym, EventKeyChar, current_modifiers(), 0);
      break;

    case SDL_KEYUP:
      printf("UP %d\n", event->key.keysym.sym);
      recordKeyboardEvent(event->key.keysym.sym, EventKeyUp, current_modifiers(), 0);
      break;

    case SDL_TEXTINPUT:
    {
      char *c = event->text.text;
      uint32_t state = 0, codepoint;
      for (; *c; c++) {
	if (!utf8_decode(&state, &codepoint, *c))
	  recordKeyboardEvent(0, EventKeyChar, 0, codepoint);
	if (state != UTF8_ACCEPT)
	  break;
      }
      break;
    }

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      mousePosition.x = event->button.x;
      mousePosition.y = event->button.y;
      if (event->type == SDL_MOUSEBUTTONDOWN)
	buttonState |= sdl_button_to_sq(event->button.button);
      else
	buttonState &= ~sdl_button_to_sq(event->button.button);
      recordMouseEvent();
      break;

    case SDL_MOUSEMOTION:
      mousePosition.x = event->motion.x;
      mousePosition.y = event->motion.y;
      recordMouseEvent();
      break;
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

static sqInt display_ioSetCursorWithMask(sqInt cursorBitsIndex, sqInt cursorMaskIndex, sqInt offsetX, sqInt offsetY)
{
  trace();

  unsigned int *cursorBits = (unsigned char *) pointerForOop(cursorBitsIndex);
  unsigned int *cursorMask = (unsigned char *) pointerForOop(cursorMaskIndex);

  SDL_Cursor *cursor = SDL_CreateCursor(cursorBits, cursorMask, 16, 16, -offsetX, -offsetY);
  if (!cursor)
    printf("Creating cursor failed: %s\n", SDL_GetError());
  SDL_SetCursor(cursor);
  return 0;
}

static sqInt display_ioSetCursorARGB(sqInt cursorBitsIndex, sqInt extentX, sqInt extentY, sqInt offsetX, sqInt offsetY)
{
  SDL_Surface *cursor_surface = surface_from_sq_bits(cursorBitsIndex, extentX, extentY, 32);
  SDL_Cursor *cursor = SDL_CreateColorCursor(cursor_surface, -offsetX, -offsetY);
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
  // fullDisplayUpdate();
  return 0;
}

static sqInt display_ioShowDisplay(sqInt dispBitsIndex, sqInt width, sqInt height, sqInt depth,
				   sqInt affectedL, sqInt affectedR, sqInt affectedT, sqInt affectedB)
{
  // trace();
  // printf("DRAW DEPTH %d (%dx%d) %d\n", depth, width, height, depth / 8 * width);
  if (width < 1 || height < 1)
    return 0;

  if (affectedL > width)  affectedL= width;
  if (affectedR > width)  affectedR= width;
  if (affectedT > height) affectedT= height;
  if (affectedB > height) affectedB= height;

  SDL_Surface *image = surface_from_sq_bits(dispBitsIndex, width, height, depth);

  SDL_Surface *screen = SDL_GetWindowSurface(window);
  SDL_BlitSurface(image, NULL, screen, NULL);
  SDL_UpdateWindowSurface(window);

  return 0;
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

static void display_winInit(void)
{
  SDL_Init(SDL_INIT_VIDEO);
}

static void display_winOpen(void)
{
  window = SDL_CreateWindow(
      "Squeak SDL2",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      640,
      480,
      SDL_WINDOW_OPENGL /*| SDL_WINDOW_ALLOW_HIGHDPI*/ | SDL_WINDOW_RESIZABLE);

  SDL_StartTextInput();

  if (!window) {
    SDL_Log("Could not create window: %s", SDL_GetError());
    SDL_Quit();
  }

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

static long  display_winImageFind(char *buf, long len)		{ trace();  return 0; }
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
