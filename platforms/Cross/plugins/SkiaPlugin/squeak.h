#ifndef SQUEAK_H
#define SQUEAK_H

extern "C" {
#define LSB_FIRST 1

#include "config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Do not include the entire sq.h file but just those parts needed. */
#include "sqConfig.h"			/* Configuration options */
#include "sqVirtualMachine.h"	/*  The virtual machine proxy definition */
#include "sqPlatformSpecific.h"	/* Platform specific definitions */
#include "sqMemoryAccess.h"

#include "handles.h"

/*** Variables ***/
extern sqInt (*nilObject)(void);

#define SQUEAK_PRIMITIVES(prefix) \
prefix sqInt (*fullDisplayUpdate)(void); \
prefix void * (*arrayValueOf)(sqInt oop); \
prefix sqInt (*booleanValueOf)(sqInt obj); \
prefix sqInt (*byteSizeOf)(sqInt oop); \
prefix sqInt (*classPoint)(void); \
prefix sqInt (*classBitmap)(void); \
prefix sqInt (*failed)(void); \
prefix void * (*fetchArrayofObject)(sqInt fieldIndex, sqInt objectPointer); \
prefix sqInt (*fetchClassOf)(sqInt oop); \
prefix sqInt (*fetchIntegerofObject)(sqInt fieldIndex, sqInt objectPointer); \
prefix sqInt (*fetchPointerofObject)(sqInt index, sqInt oop); \
prefix void * (*firstIndexableField)(sqInt oop); \
prefix sqInt (*floatObjectOf)(double  aFloat); \
prefix sqInt (*isPointers)(sqInt oop); \
prefix sqInt (*pop)(sqInt nItems); \
prefix sqInt (*popthenPush)(sqInt nItems, sqInt oop); \
prefix sqInt (*instantiateClassindexableSize)(sqInt classPointer, sqInt size); \
prefix sqInt (*positive32BitIntegerFor)(unsigned int integerValue); \
prefix sqInt (*primitiveFail)(void); \
prefix sqInt (*primitiveFailFor)(sqInt reasonCode); \
prefix sqInt (*push)(sqInt object); \
prefix sqInt (*pushInteger)(sqInt integerValue); \
prefix sqInt (*slotSizeOf)(sqInt oop); \
prefix double (*stackFloatValue)(sqInt offset); \
prefix double (*floatValueOf)(sqInt oop); \
prefix sqInt (*integerValueOf)(sqInt oop); \
prefix sqInt (*stackIntegerValue)(sqInt offset); \
prefix sqInt (*stackObjectValue)(sqInt offset); \
prefix sqInt (*stackValue)(sqInt offset); \
prefix sqInt (*sizeOfSTArrayFromCPrimitive)(void *cPtr); \
prefix sqInt (*isFloatObject)(sqInt oop); \
prefix sqInt (*isIntegerObject)(sqInt oop); \
prefix sqInt (*classString)(void);

SQUEAK_PRIMITIVES(extern)

}

#endif // SQUEAK_H
