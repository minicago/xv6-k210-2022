#ifndef __VMA_H
#define __VMA_H

#include "param.h"
#include "riscv.h"
#include "types.h"
#include "spinlock.h"
#include "file.h"
#include "fat32.h"
#include "trap.h"
#include "mmap.h"

// Per-vma state
struct vma
{
  uint64 start;
  uint64 end;
  uint64 length; // 0 means vma not used
  uint64 off;
  int permission;
  int flags;
  struct file *file;
  struct vma *next;

  struct spinlock lock;
};

extern struct vma vma_list[NVMA];

struct vma* vma_alloc();
int mmap_handler(uint64 va, int scause);
void writeback(struct vma* v, uint64 addr, int n);


#endif