#include "handles.h"

#define DEBUG_ALLOCATIONS 0

// growing array of pointers. NULL means free slot. 1-based access via toPointer. Handle value 0 means NULL ptr.
void **pointer_handles = NULL;
sqInt max_handles = 0;

void *toPointer(sqInt handle) {
#if DEBUG_ALLOCATIONS
	printf("get %li\n", handle - 1);
#endif
	return handle <= 0 || handle - 1 >= max_handles ? NULL : pointer_handles[handle - 1];
}

void freeHandle(sqInt handle) {
	if (handle < max_handles && handle > 0) {
#if DEBUG_ALLOCATIONS
		printf("freeing %li\n", handle - 1);
#endif
		pointer_handles[handle - 1] = NULL;
	}
}

sqInt toHandle(void *pointer) {
	sqInt index;

	if (pointer == NULL) {
#if DEBUG_ALLOCATIONS
		printf("NULL pointer passed!\n");
#endif
		return 0;
	}

	for (index = 0; index < max_handles; index++) {
		if (pointer_handles[index] == NULL) {
			pointer_handles[index] = pointer;
#if DEBUG_ALLOCATIONS
			printf("Assigned! %li /%li (%p)\n", index, max_handles, pointer);
#endif
			return index + 1;
		}
	}

	const int grow = 64;
	pointer_handles = (void **) realloc((void *) pointer_handles, sizeof(void *) * (max_handles + grow));
	if (!pointer_handles) {
#if DEBUG_ALLOCATIONS
		printf("REALLLOC failed\n");
#endif
		return 0;
	}

	sqInt handle = max_handles + 1;
	pointer_handles[max_handles] = pointer;
#if DEBUG_ALLOCATIONS
	printf("Assigned! %li\n", max_handles);
#endif

	for (index = max_handles + 1; index < max_handles + grow; index++)
		pointer_handles[index] = NULL;

	max_handles += grow;
#if DEBUG_ALLOCATIONS
	printf("Grew array! %li\n", max_handles);
#endif

	return handle;
}

