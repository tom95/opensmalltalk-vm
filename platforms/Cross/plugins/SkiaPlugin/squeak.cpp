#include "SkSurface.h"
#include "SkCanvas.h"
#include "SkImage.h"
#include "SkPaint.h"
#include "SkTypeface.h"

extern "C" {
#include "squeak.h"
#include "unpack.h"

sqInt primitiveGetResolvedFamilyName(void) {
    SkTypeface *typeface = static_cast<SkTypeface*>(toPointer(stackIntegerValue(0)));
    if (failed()) return primitiveFailFor(kInvalidTypeface);

    SkString s;
    typeface->getFamilyName(&s);

    Oop string = instantiateClassindexableSize(classString(), s.size());
    char *stringPtr = (char *) arrayValueOf(string);
    if (failed()) return primitiveFailFor(kInvalidString);

    std::strncpy(stringPtr, s.c_str(), s.size());

    pop(2);
    return push(string);
}

sqInt primitiveFrameEnd(void) {
    // fullDisplayUpdate();
    return 0;
}

sqInt primitiveCreateTypeface(void) {
    char *str = stringFromStack(1);
    if (!str) return primitiveFailFor(kInvalidString);

    sqInt fontStyle = stackIntegerValue(0);
    SkFontStyle::Weight weight = static_cast<SkFontStyle::Weight>((fontStyle & 0xffff) * 100);
    SkFontStyle::Width width = static_cast<SkFontStyle::Width>((fontStyle >> 16) & 0xff);
    SkFontStyle::Slant slant = static_cast<SkFontStyle::Slant>((fontStyle >> 24) & 0xff);

    free(str);

    pop(3);
    return pushInteger(toHandle(SkTypeface::MakeFromName(str,
		    SkFontStyle(weight, width, slant)).release()));
}

sqInt primitiveCreateCanvas(void) {
    sqInt surface = stackIntegerValue(0);
    if (failed())
	return primitiveFailFor(kInvalidSurface);
    sqInt canvas = toHandle(static_cast<SkSurface*>(toPointer(surface))->getCanvas());
    static_cast<SkCanvas*>(toPointer(canvas))->drawColor(SK_ColorRED);
    pop(2);
    return pushInteger(canvas);
}
// signed char primitiveCreateCanvasAccessorDepth = 0;

sqInt primitiveCreateSurface(void) {
    sqInt width, height, depth;
    void *buffer;

    if (!formFromStack(0, &width, &height, &depth, &buffer))
	return primitiveFailFor(kInvalidForm);
    if (width < 0 || height < 0)
	return primitiveFailFor(kEmptyForm);

    SkImageInfo info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kUnpremul_SkAlphaType);
    sqInt surface = toHandle(SkSurface::MakeRasterDirect(info, buffer, width * 4).release());
    if (surface == 0)
	return primitiveFailFor(kInvalidSurface);

    pop(2);
    return pushInteger(surface);
}

sqInt primitiveCreateImageFromForm(void) {
    sqInt width, height, depth;
    void *buffer;

    if (!formFromStack(0, &width, &height, &depth, &buffer))
	return primitiveFailFor(kInvalidForm);
    if (width < 0 || height < 0)
	return primitiveFailFor(kEmptyForm);

    static SkImageInfo info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kUnpremul_SkAlphaType);
    sqInt stride = width * depth / 8;

    // SkData *data = SkData::MakeWithoutCopy(bmBuffer, bmWidth * stride).release();
    // sqInt image = toHandle(SkImage::MakeRasterData(info, sk_ref_sp(data), stride).release());
    SkPixmap pixmap(info, buffer, stride);
    sqInt image = toHandle(SkImage::MakeRasterCopy(pixmap).release());

    pop(2);
    return pushInteger(image);
}

sqInt primitiveCanvasDrawLine(void) {
    SkCanvas *canvas = canvasFromStack(3);
    if (!canvas) return primitiveFailFor(100);

    point_t start;
    if (!pointFromStack(2, &start))
	return primitiveFailFor(kInvalidPoint);
    point_t end;
    if (!pointFromStack(1, &end))
	return primitiveFailFor(kInvalidPoint);

    SkPaint paint;
    if (!paintFromStack(0, &paint))
	return primitiveFailFor(kInvalidPaint);

    canvas->drawLine(start.x, start.y, end.x, end.y, paint);

    pop(4);
    return 0;
}

sqInt primitiveCanvasDrawRect(void) {
    SkCanvas *canvas = canvasFromStack(1);

    sqInt rectOop = stackObjectValue(2);
    sqInt origin = fetchPointerofObject(0, rectOop);
    sqInt corner = fetchPointerofObject(1, rectOop);
    if (failed()) { primitiveFailFor(100); }

    SkRect rect = SkRect::MakeLTRB(
	integerValueOf(fetchPointerofObject(0, origin)),
	integerValueOf(fetchPointerofObject(1, origin)),
	integerValueOf(fetchPointerofObject(0, corner)),
	integerValueOf(fetchPointerofObject(1, corner))
    );
    if (failed()) { primitiveFailFor(101); }

    SkPaint paint;
    if (!paintFromStack(0, &paint))
	return primitiveFailFor(kInvalidPaint);

    canvas->drawRect(rect, paint);
    pop(3);
    return 0;
}

sqInt primitiveCanvasRotate(void) {
    SkCanvas *canvas = canvasFromStack(1);
    if (!canvas)
	return primitiveFailFor(kInvalidCanvas);

    double radians = stackFloatValue(0);
    if (failed())
	return primitiveFailFor(kInvalidNumber);

    canvas->rotate(radians);
    pop(2);
    return 0;
}

sqInt primitiveCanvasScale(void) {
    SkCanvas *canvas = canvasFromStack(1);
    if (!canvas)
	return primitiveFailFor(kInvalidCanvas);

    point_t scale;
    if (!pointFromStack(0, &scale))
	return primitiveFailFor(kInvalidPoint);

    canvas->scale(scale.x, scale.y);

    pop(2);
    return 0;
}

sqInt primitiveCanvasTranslate(void) {
    SkCanvas *canvas = canvasFromStack(1);
    if (!canvas)
	return primitiveFailFor(kInvalidCanvas);

    point_t origin;
    if (!pointFromStack(0, &origin))
	return primitiveFailFor(kInvalidPoint);

    canvas->translate(origin.x, origin.y);

    pop(2);
    return 0;
}

sqInt primitiveCanvasSave(void) {
    SkCanvas *canvas = canvasFromStack(0);
    if (!canvas)
	return primitiveFailFor(kInvalidCanvas);

    canvas->save();
    pop(1);
    return 0;
}

sqInt primitiveCanvasRestore(void) {
    SkCanvas *canvas = canvasFromStack(0);
    if (!canvas)
	return primitiveFailFor(kInvalidCanvas);

    canvas->restore();
    pop(1);
    return 0;
}

sqInt primitiveCanvasDrawImage(void) {
    SkCanvas *canvas = canvasFromStack(2);
    if (!canvas)
	return primitiveFailFor(kInvalidCanvas);

    point_t origin;
    if (!pointFromStack(1, &origin))
	return primitiveFailFor(kInvalidPoint);

    SkImage *image = imageFromStack(3);
    if (!image)
	return primitiveFailFor(kInvalidImage);

    SkPaint paint;
    if (!paintFromStack(0, &paint))
	return primitiveFailFor(kInvalidPaint);

    canvas->drawImage(image, origin.x, origin.y, &paint);
    pop(4);
    return 0;
}

sqInt primitiveCanvasDrawText(void) {
    SkCanvas *canvas = canvasFromStack(2);
    if (!canvas)
	return primitiveFailFor(kInvalidCanvas);

    point_t origin;
    if (!pointFromStack(1, &origin))
	return primitiveFailFor(kInvalidPoint);

    SkPaint paint;
    if (!paintFromStack(0, &paint))
	return primitiveFailFor(kInvalidPaint);

    Oop string = stackObjectValue(3);
    char *data = (char *) firstIndexableField(string);
    sqInt len = byteSizeOf(string);
    if (failed()) return primitiveFailFor(kInvalidString);

    canvas->drawText(data, len, origin.x, origin.y, paint);

    pop(4);
    return 0;
}

struct VirtualMachine* interpreterProxy;

sqInt setInterpreter(struct VirtualMachine*anInterpreter) {
	interpreterProxy = anInterpreter;
	sqInt ok = ((interpreterProxy->majorVersion()) == (VM_PROXY_MAJOR))
		&& ((interpreterProxy->minorVersion()) >= (VM_PROXY_MINOR));
	return ok;
}

static const char *moduleName = "SkiaPlugin *  (i)";

const char* primitiveModuleName(void) { return moduleName; }

static char _m[] = "SkiaPlugin";
const void* SkiaPlugin_exports[][3] = {
	{(void*)_m, "primitiveCanvasDrawImage\000\000", (void*)primitiveCanvasDrawImage},
	{(void*)_m, "primitiveCanvasDrawLine\000\000", (void*)primitiveCanvasDrawLine},
	{(void*)_m, "primitiveCanvasDrawRect\000\000", (void*)primitiveCanvasDrawRect},
	{(void*)_m, "primitiveCanvasDrawText\000\000", (void*)primitiveCanvasDrawText},
	{(void*)_m, "primitiveCanvasRestore\000\000", (void*)primitiveCanvasRestore},
	{(void*)_m, "primitiveCanvasSave\000\000", (void*)primitiveCanvasSave},
	{(void*)_m, "primitiveCanvasScale\000\000", (void*)primitiveCanvasScale},
	{(void*)_m, "primitiveCanvasTranslate\000\000", (void*)primitiveCanvasTranslate},
	{(void*)_m, "primitiveCreateCanvas\000\000", (void*)primitiveCreateCanvas},
	{(void*)_m, "primitiveCreateImageFromForm\000\000", (void*)primitiveCreateImageFromForm},
	{(void*)_m, "primitiveCreateSurface\000\000", (void*)primitiveCreateSurface},
	{(void*)_m, "primitiveCreateTypeface\000\000", (void*)primitiveCreateTypeface},
	{(void*)_m, "primitiveFrameEnd\000\000", (void*)primitiveFrameEnd},
	{(void*)_m, "primitiveGetResolvedFamilyName\000\000", (void*)primitiveGetResolvedFamilyName},
	{(void*)_m, "primitiveModuleName\000\000", (void*)primitiveModuleName},
	{(void*)_m, "setInterpreter", (void*)setInterpreter},
	// {(void*)_m, "initialiseModule", (void*)initialiseModule},
	// {(void*)_m, "shutdownModule\000\377", (void*)shutdownModule},
	{NULL, NULL, NULL}
};

}
