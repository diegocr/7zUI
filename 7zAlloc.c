/* 7zAlloc.c */

#include <stdlib.h>
#include "7zAlloc.h"

void *SzAlloc(size_t size)
{
	return((size > 0) ? AllocVec(size, MEMF_ANY) : 0);
}

void SzFree(void *address)
{
	if(address != 0)
		FreeVec( address );
}
