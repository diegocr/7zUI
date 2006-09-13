/* 7zAlloc.h */

#ifndef __7Z_ALLOC_H
#define __7Z_ALLOC_H

#include <stddef.h>
#include <proto/exec.h>

typedef struct _ISzAlloc
{
  void *(*Alloc)(size_t size);
  void (*Free)(void *address); /* address can be 0 */
} ISzAlloc;

void *SzAlloc(size_t size);
void SzFree(void *address);

#define SzAllocTemp	SzAlloc
#define SzFreeTemp	SzFree

#endif
