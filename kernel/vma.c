#include "include/param.h"
#include "include/types.h"
#include "include/memlayout.h"
#include "include/elf.h"
#include "include/riscv.h"
#include "include/vm.h"
#include "include/kalloc.h"
#include "include/proc.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/vma.h"


struct vma vma_list[NVMA];
struct vma* vma_alloc(){
  for(int i = 0; i < NVMA; i++){
    acquire(&vma_list[i].lock);
    if(vma_list[i].length == 0){
      return &vma_list[i];
    }else{
      release(&vma_list[i].lock);
    }
  }
  panic("no enough vma");
}

int
mmap_handler(uint64 va, int scause)
{
  struct proc *p = myproc();
  struct vma* v = p->vma;
  while(v != 0){
    if(va >= v->start && va < v->end){
      break;
    }
    v = v->next;
  }

  if(v == 0) return -1; // not mmap addr
  if(scause == 13 && !(v->permission & PTE_R)) return -2; // unreadable vma
  if(scause == 15 && !(v->permission & PTE_W)) return -3; // unwritable vma

  // load page from file
  va = PGROUNDDOWN(va);
  char* mem = kalloc();
  if (mem == 0) return -4; // kalloc failed
  
  memset(mem, 0, PGSIZE);

  if(mappages(p->pagetable, va, PGSIZE, (uint64)mem, v->permission) != 0){
    kfree(mem);
    return -5; // map page failed
  }

  struct file *f = v->file;
  elock(f->ep);
  eread(f->ep, 0, (uint64)mem, v->off + va - v->start, PGSIZE);
  eunlock(f->ep);
  return 0;
}

void
writeback(struct vma* v, uint64 addr, int n)
{
  if(!(v->permission & PTE_W) || (v->flags & MAP_PRIVATE)) // no need to writeback
    return;

  if((addr % PGSIZE) != 0)
    panic("unmap: not aligned");

  printf("starting writeback: %p %d\n", addr, n);

  struct file* f = v->file;

  int max = ((MAXOPBLOCKS-1-1-2) / 2) * FSSIZE;
  int i = 0;
  while(i < n){
    int n1 = n - i;
    if(n1 > max)
      n1 = max;

    elock(f->ep);
    printf("%p %d %d\n",addr + i, v->off + v->start - addr, n1);
    int r = ewrite(f->ep, 1, addr + i, v->off + v->start - addr + i, n1);
    eunlock(f->ep);
    i += r;
  }
}

