#include "GrBackendSurface.h"
#include "GrContext.h"
#include "SDL.h"
#include "SkCanvas.h"
#include "SkTypeface.h"
#include "SkRandom.h"
#include "SkSurface.h"

#include "gl/GrGLInterface.h"
#include "gl/GrGLUtil.h"

#include <GL/gl.h>

#include "common.h"

#define WIDTH 640
#define HEIGHT 480

extern "C" {

static const int kStencilBits = 8;  // Skia needs 8 stencil bits

void destroy_context(ApplicationState *state) {
    state->context->abandonContext();
}

void draw_window(ApplicationState *state) {
    SkPaint paint;
    paint.setFilterQuality(kNone_SkFilterQuality);
    paint.setAntiAlias(false);

    state->canvas->drawBitmap(*state->window_bitmap, 0, 0, &paint);
    state->canvas->flush();
}

void blit(ApplicationState *state, char *bits, int width, int height, int depth,
        int l, int r, int t, int b) {
    static SkImageInfo info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kUnpremul_SkAlphaType);
    // sk_sp<SkData> data = SkData::MakeWithoutCopy(bits, width * height * depth / 8);
    // sk_sp<SkImage> image = SkImage::MakeRasterData(info, data, width * depth / 8);
    // state->canvas->drawImage(image, 0, 0);

    SkPixmap pixmap(info, bits, width * depth);

    // printf("\n\n\n%i %i %i %i\n\n\n\n", l, r, t, b);
    state->window_bitmap->installPixels(info, bits, width * depth / 8, nullptr, nullptr);

    // SkRect rect = SkRect::MakeLTRB(l, t, r, b);
    // state->canvas->drawBitmapRect(*state->window_bitmap, rect, rect, nullptr);
    draw_window(state);
}

void init_context(ApplicationState *state) {
    auto interface = GrGLMakeNativeInterface();
    // setup contexts
    sk_sp<GrContext> context = GrContext::MakeGL(interface);
    SkASSERT(context);

    SkSafeRef(&*context);
    state->context = &*context;

    // Wrap the frame buffer object attached to the screen in a Skia render target so Skia can
    // render to it
    GrGLint buffer;
    GR_GL_GetIntegerv(interface.get(), GR_GL_FRAMEBUFFER_BINDING, &buffer);
    GrGLFramebufferInfo info;
    info.fFBOID = (GrGLuint) buffer;
    SkColorType colorType;

    uint32_t windowFormat = SDL_GetWindowPixelFormat(state->window);
    int contextType;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &contextType);

    int width, height;
    SDL_GL_GetDrawableSize(state->window, &width, &height);

    glViewport(0, 0, width, height);
    glClearColor(1, 1, 1, 1);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (SDL_PIXELFORMAT_RGBA8888 == windowFormat) {
        info.fFormat = GR_GL_RGBA8;
        colorType = kRGBA_8888_SkColorType;
    } else {
        SkASSERT(SDL_PIXELFORMAT_BGRA8888);
        colorType = kBGRA_8888_SkColorType;
        if (SDL_GL_CONTEXT_PROFILE_ES == contextType) {
            info.fFormat = GR_GL_BGRA8;
        } else {
            // We assume the internal format is RGBA8 on desktop GL
            info.fFormat = GR_GL_RGBA8;
        }
    }

    static const int kMsaaSampleCount = 0; //4;
    // If you want multisampling, uncomment the below lines and set a sample count
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, kMsaaSampleCount);
    GrBackendRenderTarget target(width, height, kMsaaSampleCount, kStencilBits, info);

    SkSurfaceProps props(SkSurfaceProps::kLegacyFontHost_InitType);
    sk_sp<SkSurface> surface(SkSurface::MakeFromBackendRenderTarget(context.get(), target,
                                                                    kBottomLeft_GrSurfaceOrigin,
                                                                    colorType, nullptr, &props));

    SkSafeRef(&*surface);
    state->surface = &*surface;

    state->canvas = surface->getCanvas();
    state->window_bitmap = new SkBitmap();
}

void draw_text(ApplicationState *state, int x, int y) {
    const char* helpMessage = "Click and drag to create rects. Press esc to quit.";

    SkPaint paint;
    paint.setTextSize(32);
    paint.setLCDRenderText(true);
    paint.setAntiAlias(true);
    paint.setTypeface(SkTypeface::MakeFromName("Roboto Mono", SkFontStyle::Normal()));
    paint.setHinting(SkPaint::kFull_Hinting);

    paint.setColor(SK_ColorBLACK);
    state->canvas->drawText(helpMessage, strlen(helpMessage), SkIntToScalar(x), SkIntToScalar(y), paint);
}

}
