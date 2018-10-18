#ifndef HANDLES_H
#define HANDLES_H

extern "C" {

#define LSB_FIRST 1
#include "sqConfig.h"			/* Configuration options */
#include "sqVirtualMachine.h"	/*  The virtual machine proxy definition */
#include "sqPlatformSpecific.h"	/* Platform specific definitions */
#include "sqMemoryAccess.h"
#include <stdlib.h>

sqInt toHandle(void *pointer);
void freeHandle(sqInt handle);
void *toPointer(sqInt handle);

}
#endif // HANDLES_H
