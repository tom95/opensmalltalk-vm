#ifndef UNPACK_H
#define UNPACK_H

#include "SkCanvas.h"
#include "SkTypeface.h"
extern "C" {
#include "squeak.h"

const sqInt kInvalidPoint = 111;
const sqInt kInvalidForm = 112;
const sqInt kInvalidNumber = 113;
const sqInt kInvalidPaint = 114;
const sqInt kInvalidString = 115;
const sqInt kInvalidTypeface = 115;
const sqInt kInvalidSurface = 116;
const sqInt kEmptyForm = 117;
const sqInt kInvalidCanvas = 118;
const sqInt kInvalidImage = 119;

typedef sqInt Oop;

typedef struct point {
    double x;
    double y;
} point_t;


SkCanvas *canvasFromStack(sqInt index);
SkImage *imageFromStack(sqInt index);
char *stringFromStack(sqInt index);

bool formFromStack(sqInt index, sqInt *width, sqInt* height, sqInt *depth, void **buffer);
bool pointFromStack(sqInt index, point_t *point);
bool paintFromStack(sqInt index, SkPaint *paint);
}

#endif // UNPACK_H
