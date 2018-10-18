#ifndef SKIA_PLUGIN_H__
#define SKIA_PLUGIN_H__

#include "sk_data.h"
#include "sk_image.h"
#include "sk_canvas.h"
#include "sk_surface.h"
#include "sk_paint.h"
#include "sk_path.h"
#include "sk_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_typeface_t sk_typeface_t;
typedef uint32_t sk_font_style_t;

#ifndef __cplusplus
// sq.h has troubles being included in c++ land, it will automatically
// be included in c land. So only define these in c land.
void *toPointer(sqInt);
sqInt toHandle(void*);
void freeHandle(sqInt);
#endif

sk_font_style_t sk_font_style_create(int, int, int);
sk_typeface_t *sk_typeface_new_default();
sk_typeface_t *sk_typeface_new_from_name(const char*, sk_font_style_t);
void sk_paint_set_typeface_from_name(sk_paint_t*, const char*, sk_font_style_t);
sk_typeface_t *sk_typeface_new_from_file(const char*, int index);
void sk_typeface_unref(sk_typeface_t*);
void sk_paint_set_typeface(sk_paint_t*, sk_typeface_t*);
void sk_paint_set_text_size(sk_paint_t*, double size);
void sk_canvas_draw_text(sk_canvas_t*, const char*, size_t, double, double, sk_paint_t*);
void sk_canvas_draw_color(sk_canvas_t*, uint32_t, unsigned int);
void sk_canvas_draw_rounded_rect(sk_canvas_t*, double, double, double, double, double, double, sk_paint_t*);
void sk_canvas_draw_line(sk_canvas_t*, double, double, double, double, sk_paint_t*);
void sk_canvas_copy_transform(sk_canvas_t*, sk_canvas_t*);
void sk_canvas_reset_matrix(sk_canvas_t*);
double sk_paint_measure_text(sk_paint_t *paint, char *text, size_t length);
void sk_paint_get_font_metrics(sk_paint_t *paint, long int metricsObj);
void sk_canvas_draw_rounded_rect_radii(sk_canvas_t*, double, double, double, double, double, double, double, double, double, double, double, double, sk_paint_t*);

#ifdef __cplusplus
}
#endif

#endif // SKIA_PLUGIN_H__
