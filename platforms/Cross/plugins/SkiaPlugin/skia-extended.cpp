#include "SkiaPlugin.h"

#include "SkTypeface.h"
#include "SkFontStyle.h"
#include "SkPaint.h"
#include "SkCanvas.h"
#include "SkRRect.h"

extern "C" {

// weight: [0, 100, 200, ..., 1000], width: [1, 2, ..., 9], slant: 0->upright, 1->italic, 2->oblique
sk_font_style_t sk_font_style_create(int weight, int width, int slant) {
	return weight + (width << 16) + (slant << 24);
}

sk_typeface_t *sk_typeface_new_default() {
	return (sk_typeface_t *) SkTypeface::MakeDefault().release();
}

sk_typeface_t *sk_typeface_new_from_name(const char *name, sk_font_style_t style) {
	// FIXME not sure if we need to deconstruct this everytime, format is already correct
	SkFontStyle fstyle(style & 0xFFFF, (style >> 16) & 0xFF, (SkFontStyle::Slant) ((style >> 24) & 0xFF));

	return (sk_typeface_t *) SkTypeface::MakeFromName(name, fstyle).release();
}

void sk_paint_set_typeface_from_name(sk_paint_t *paint, const char *name, sk_font_style_t style) {
	SkFontStyle fstyle(style & 0xFFFF, (style >> 16) & 0xFF, (SkFontStyle::Slant) ((style >> 24) & 0xFF));
	((SkPaint *) paint)->setTypeface(SkTypeface::MakeFromName(name, fstyle));
}

sk_typeface_t *sk_typeface_new_from_file(const char *path, int index) {
	return (sk_typeface_t *) SkTypeface::MakeFromFile(path, index).release();
}

void sk_typeface_unref (sk_typeface_t *typeface) {
	SkSafeUnref((SkTypeface *) typeface);
}

void sk_paint_set_typeface(sk_paint_t *paint, sk_typeface_t *typeface) {
	auto p = (SkPaint *) paint;
	// SkSafeUnref(p->getTypeface());

	// danger zone: figure out how to combat the shared pointer
	p->setTypeface(sk_sp<SkTypeface>((SkTypeface *) typeface));
}

void sk_paint_set_text_size(sk_paint_t *paint, double size) {
	((SkPaint *) paint)->setTextSize(size);
}

void sk_paint_set_text_encoding_utf32(sk_paint_t *paint) {
	((SkPaint *) paint)->setTextEncoding(SkPaint::kUTF32_TextEncoding);
}

void sk_canvas_draw_text(sk_canvas_t *canvas, const char *text, size_t len, double x, double y, sk_paint_t *paint) {
	((SkCanvas *) canvas)->drawText(text, len, x, y, reinterpret_cast<const SkPaint &>(*paint));
}

// 0->clear, 1->src, 2->dst, 3->srcOver, 4->dstOver, 5->srcIn, 6->dstIn, ... include/core/SkBlendMode.h
void sk_canvas_draw_color(sk_canvas_t *canvas, uint32_t color, unsigned int blend_mode) {
	((SkCanvas *) canvas)->drawColor(color, (SkBlendMode) blend_mode);
}

void sk_canvas_draw_rounded_rect(sk_canvas_t *canvas, double left, double right, double top, double bottom, double radius_x, double radius_y, sk_paint_t *paint) {
	SkRRect round_rect;
	round_rect.setRectXY(SkRect::MakeLTRB(left, top, right, bottom), radius_x, radius_y);

	((SkCanvas *) canvas)->drawRRect(round_rect, reinterpret_cast<const SkPaint &>(*paint));
}

void sk_canvas_draw_rounded_rect_radii(sk_canvas_t *canvas,
		double left, double right, double top, double bottom,
		double tlX, double tlY, double trX, double trY, double lrX, double lrY, double llX, double llY,
		sk_paint_t *paint) {
	SkRRect round_rect;
	SkVector radii[4];
	radii[0].fX = tlX; radii[0].fY = tlY;
	radii[1].fX = trX; radii[1].fY = trY;
	radii[2].fX = lrX; radii[2].fY = lrY;
	radii[3].fX = llX; radii[3].fY = llY;

	round_rect.setRectRadii(SkRect::MakeLTRB(left, top, right, bottom), radii);
}

void sk_canvas_draw_line(sk_canvas_t *canvas, double x0, double y0, double x1, double y1, sk_paint_t *paint) {
	((SkCanvas *) canvas)->drawLine(x0, y0, x1, y1, reinterpret_cast<const SkPaint &>(*paint));
}

void sk_canvas_copy_transform(sk_canvas_t *from, sk_canvas_t *to) {
	auto matrix = ((SkCanvas *) from)->getTotalMatrix();
	((SkCanvas *) to)->setMatrix(matrix);
}

void sk_canvas_reset_matrix(sk_canvas_t *canvas) {
	((SkCanvas *) canvas)->resetMatrix();
}

double sk_paint_measure_text(sk_paint_t *paint, char *text, size_t length) {
	return ((SkPaint *) paint)->measureText(text, length);
}

// FIXME bit tricky here. we dont necessarily wanna create a copy of this struct everytime, just
// so we can pass it to C, so we declare some squeak funcs in C++ land. probably not optimal.
extern void storePointerofObjectwithValue(long int, long int, long int);
extern long int floatObjectOf(double);
void sk_paint_get_font_metrics(sk_paint_t *paint, long int metricsObj) {
	SkPaint::FontMetrics metrics;
	double lineSpacing = ((SkPaint *) paint)->getFontMetrics(&metrics);

	// skia goes negative/upwards, so we have to inverse y
	storePointerofObjectwithValue(0, metricsObj, floatObjectOf(-metrics.fTop));
	storePointerofObjectwithValue(1, metricsObj, floatObjectOf(-metrics.fAscent));
	storePointerofObjectwithValue(2, metricsObj, floatObjectOf(-metrics.fDescent));
	storePointerofObjectwithValue(3, metricsObj, floatObjectOf(-metrics.fBottom));
	storePointerofObjectwithValue(4, metricsObj, floatObjectOf(metrics.fLeading));
	storePointerofObjectwithValue(5, metricsObj, floatObjectOf(metrics.fAvgCharWidth));
	storePointerofObjectwithValue(6, metricsObj, floatObjectOf(metrics.fMaxCharWidth));
	storePointerofObjectwithValue(7, metricsObj, floatObjectOf(metrics.fXMin));
	storePointerofObjectwithValue(8, metricsObj, floatObjectOf(metrics.fXMax));
	storePointerofObjectwithValue(9, metricsObj, floatObjectOf(metrics.fXHeight));
	storePointerofObjectwithValue(10, metricsObj, floatObjectOf(metrics.fCapHeight));
	storePointerofObjectwithValue(11, metricsObj, floatObjectOf(metrics.fUnderlineThickness));
	storePointerofObjectwithValue(12, metricsObj, floatObjectOf(metrics.fUnderlinePosition));
	storePointerofObjectwithValue(13, metricsObj, floatObjectOf(metrics.fStrikeoutThickness));
	storePointerofObjectwithValue(14, metricsObj, floatObjectOf(metrics.fStrikeoutPosition));
	storePointerofObjectwithValue(15, metricsObj, floatObjectOf(lineSpacing));
}

long int sq_color_type_for_depth(long int depth) {
	// TODO verify this
	switch (depth) {
		case 32:
			return (long int) kBGRA_8888_SkColorType;
		case 24:
			// FIXME probably wrong
			return (long int) kRGB_888x_SkColorType;
		case 16:
			return (long int) kRGB_565_SkColorType;
		case 8:
		case 4:
		case 2:
		case 1:
			// FIXME not sure what to do here
			return (long int) kAlpha_8_SkColorType;
		default:
			return (long int) kUnknown_SkColorType;
	}
}
long int sq_alpha_type_for_depth(long int depth) {
	switch (depth) {
		case 32:
			return (long int) kUnpremul_SkAlphaType;
		case 24:
		case 16:
			return (long int) kOpaque_SkAlphaType;
		case 8:
		case 4:
		case 2:
		case 1:
			return (long int) kPremul_SkAlphaType;
		default:
			return (long int) kUnknown_SkAlphaType;
	}
}

int sk_color_type_bytes_per_pixel(long int color_type) {
	return SkColorTypeBytesPerPixel((SkColorType) color_type);
}

}

