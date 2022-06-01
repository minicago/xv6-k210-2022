#ifndef _VMA_H
#define _VMA_H

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

struct vma vma_list[NVMA];
struct vma* vma_alloc();
uint64 sys_mmap(void);
uint64 sys_munmap(void);

#endif