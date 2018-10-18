#include "unpack.h"

SkCanvas *canvasFromStack(sqInt index) {
    sqInt handle = stackIntegerValue(index);
    if (failed()) {
	return NULL;
    }
    return static_cast<SkCanvas*>(toPointer(handle));
}

SkImage *imageFromStack(sqInt index) {
    sqInt handle = stackIntegerValue(index);
    if (failed()) {
	return NULL;
    }
    return static_cast<SkImage*>(toPointer(handle));
}

char *stringFromStack(sqInt index) {
    Oop string = stackObjectValue(index);
    char *data = (char *) firstIndexableField(string);
    sqInt len = byteSizeOf(string);
    if (failed())
	return NULL;

    char *str_nullterm = (char *) malloc(sizeof(char) * (len + 1));
    if (!str_nullterm)
	return NULL;
    std::strncpy(str_nullterm, data, len);
    str_nullterm[len] = '\0';

    return str_nullterm;
}

bool formFromStack(sqInt index, sqInt *width, sqInt* height, sqInt *depth, void **buffer) {
    Oop oop = stackObjectValue(index);
    if (!isPointers(oop) || slotSizeOf(oop) < 5)
	return false;

    Oop bitsOop = fetchPointerofObject(0, oop);
    if (fetchClassOf(bitsOop) != classBitmap())
	return false;

    *width = fetchIntegerofObject(1, bitsOop);
    *height = fetchIntegerofObject(2, bitsOop);
    *depth = fetchIntegerofObject(3, bitsOop);
    *buffer = (void *) (fetchArrayofObject(0, bitsOop));

    if (failed() || *depth != 32)
	return false;

    return true;
}

bool pointFromStack(sqInt index, point_t *point) {
    Oop oop = stackObjectValue(index);
    if (fetchClassOf(stackObjectValue(1)) != classPoint())
	return false;

    Oop valueOop = fetchPointerofObject(0, oop);
    if (isFloatObject(valueOop))
	point->x = floatValueOf(valueOop);
    else if (isIntegerObject(valueOop))
	point->x = integerValueOf(valueOop);
    else
	return false;

    valueOop = fetchPointerofObject(1, oop);
    if (isFloatObject(valueOop))
	point->y = floatValueOf(valueOop);
    else if (isIntegerObject(valueOop))
	point->y = integerValueOf(valueOop);
    else
	return false;

    return true;
}

bool paintFromStack(Oop sqPaintOop, SkPaint *paint) {
    void *paintData = (void*) fetchArrayofObject(0, sqPaintOop);
    if (failed()) {
	return false;
    }

    const uint32_t* uint_data = static_cast<const uint32_t*>(paintData);
    const float* float_data = static_cast<const float*>(paintData);

    paint->setAntiAlias(uint_data[0] != 0);
    paint->setColor(uint_data[1]);
    paint->setBlendMode(static_cast<SkBlendMode>(uint_data[2]));
    paint->setStyle(static_cast<SkPaint::Style>(uint_data[3]));
    paint->setStrokeWidth(float_data[4]);

    Oop typefaceOop = fetchPointerofObject(1, sqPaintOop);

    if (typefaceOop != nilObject()) {
	SkTypeface *typeface = static_cast<SkTypeface*>(toPointer(fetchIntegerofObject(2, typefaceOop)));
	if (failed()) return false;

	paint->setTypeface(sk_ref_sp(typeface));
    }
    if (failed())
	return false;

    paint->setTextSize(float_data[5]);
    paint->setSubpixelText(true);
    paint->setHinting(SkPaint::kSlight_Hinting);

    return true;
}
