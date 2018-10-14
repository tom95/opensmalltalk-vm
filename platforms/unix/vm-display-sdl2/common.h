#if __cplusplus
extern "C" {
#endif
typedef struct SkCanvas SkCanvas;
typedef struct GrContext GrContext;
typedef struct SkSurface SkSurface;
typedef struct SkBitmap SkBitmap;

typedef struct ApplicationState {
    SkCanvas *canvas;
    GrContext *context;
    SkSurface *surface;
    SDL_Window *window;
    SkBitmap *window_bitmap;
} ApplicationState;

void destroy_context(ApplicationState *state);
void init_context(ApplicationState *state);
void draw_text(ApplicationState *state, int x, int y);
void draw_window(ApplicationState *state);
void blit(ApplicationState *state, char *bits, int width, int height, int depth, int l, int r, int t, int b);
void begin_draw(ApplicationState *state);
void end_draw(ApplicationState *state);
#if __cplusplus
}
#endif
