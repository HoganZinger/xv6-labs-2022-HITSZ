// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

// create a memory pool for each CPU
//we can reuse a single memory pool
//allocate memory pool by CPU id
struct kmem kmem_all[NCPU];

void
kinit()
{
  //initialize locks
  for(int i = 0; i < NCPU; i++){
  initlock(&(kmem_all[i].lock), "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
// CHANGE: free the page of physical memory to the memory pool of each CPU
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int cpu_id = cpuid();
  acquire(&(kmem_all[cpu_id].lock));
  r->next = kmem_all[cpu_id].freelist;
  kmem_all[cpu_id].freelist = r;
  release(&(kmem_all[cpu_id].lock));
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();

  int cpu_id = cpuid();
  acquire(&(kmem_all[cpu_id].lock));
  r = kmem_all[cpu_id].freelist;
  //find a free memory block from the memory pool of cpu itself
  if(r){
    kmem_all[cpu_id].freelist = r->next;
    }
  //try to find from the memory block of another cpu
  else{
    for(int i = 0; i < NCPU; i++)
    {
      if(i == cpu_id) continue;
      //lock before checking
      acquire(&(kmem_all[i].lock));

      r = kmem_all[i].freelist;
      if(r)
      {
        kmem_all[i].freelist = r->next;
        release(&(kmem_all[i].lock));
        break;
      }

      release(&(kmem_all[i].lock));
    }
  }
  release(&(kmem_all[cpu_id].lock));

  pop_off();
  
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
