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

uint64
sys_mmap(void)
{
  uint64 addr;
  int length, prot, flags, fd, offset;
  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0 || argint(2, &prot) < 0 || argint(3, &flags) < 0 || argint(4, &fd) < 0 || argint(5, &offset) < 0){
    return -1;
  }

  if(addr != 0)
    panic("mmap: addr not 0");
  if(offset != 0)
    panic("mmap: offset not 0");

  struct proc *p = myproc();
  struct file* f = p->ofile[fd];

  int pte_flag = PTE_U;
  if (prot & PROT_WRITE) {
    if(!f->writable && !(flags & MAP_PRIVATE)) return -1; // map to a unwritable file with PROT_WRITE
    pte_flag |= PTE_W;
  }
  if (prot & PROT_READ) {
    if(!f->readable) return -1; // map to a unreadable file with PROT_READ
    pte_flag |= PTE_R;
  }

  struct vma* v = vma_alloc();
  v->permission = pte_flag;
  v->length = length;
  v->off = offset;
  v->file = myproc()->ofile[fd];
  v->flags = flags;
  filedup(f);
  struct vma* pv = p->vma;
  if(pv == 0){
    v->start = VMA_START;
    v->end = v->start + length;
    p->vma = v;
  }else{
    while(pv->next) pv = pv->next;
    v->start = PGROUNDUP(pv->end);
    v->end = v->start + length;
    pv->next = v;
    v->next = 0;
  }
  addr = v->start;
  printf("mmap: [%p, %p)\n", addr, v->end);

  release(&v->lock);
  return addr;
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

uint64
sys_munmap(void)
{
  uint64 addr;
  int length;
  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0){
    return -1;
  }

  struct proc *p = myproc();
  struct vma *v = p->vma;
  struct vma *pre = 0;
  while(v != 0){
    if(addr >= v->start && addr < v->end) break; // found
    pre = v;
    v = v->next;
  }

  if(v == 0) return -1; // not mapped
  printf("munmap: %p %d\n", addr, length);
  if(addr != v->start && addr + length != v->end) panic("munmap middle of vma");

  if(addr == v->start){
    writeback(v, addr, length);
    uvmunmap(p->pagetable, addr, length / PGSIZE, 1);
    if(length == v->length){
      // free all
      fileclose(v->file);
      if(pre == 0){
        p->vma = v->next; // head
      }else{
        pre->next = v->next;
        v->next = 0;
      }
      acquire(&v->lock);
      v->length = 0;
      release(&v->lock);
    }else{
      // free head
      v->start -= length;
      v->off += length;
      v->length -= length;
    }
  }else{
    // free tail
    v->length -= length;
    v->end -= length;
  }
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

    begin_op();
    elock(f->ep);
    printf("%p %d %d\n",addr + i, v->off + v->start - addr, n1);
    int r = writei(f->ep, 1, addr + i, v->off + v->start - addr + i, n1);
    iunlock(f->ep);
    end_op();
    i += r;
  }
}

