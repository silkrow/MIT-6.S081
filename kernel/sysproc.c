#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
// Three arguments. 
// 1) the starting virtual address of the first user page to check.
// 2) the number of pages to check.
// 3) a user address to a buffer to store the results into a bitmask.
  // lab pgtbl: your code here.
  uint64 v_st;
  int npages;
  uint64 u_addr;
  struct proc *p = myproc();
  uint64 ans = 0;

  if(argaddr(0, &v_st) < 0) return -1;
  if(argint(1, &npages) < 0) return -1;
  if(argaddr(2, &u_addr) < 0) return -1;

  uint64 va = v_st;
  pagetable_t pagetable = p->pagetable;

  for (int i = 0; i < npages; i++){
	pte_t *pte;
    for(int level = 2; level >= 0; level--) {
      pte = &pagetable[PX(level, va)];
      if(*pte & PTE_V) {
        pagetable = (pagetable_t)PTE2PA(*pte);
      } else {
		break;
      }
    }
	if((*pte & PTE_V) && (*pte & PTE_A)){
		//printf("pte %p pa %p va %p\n", *pte, PTE2PA(*pte), va);
		ans |= 1<<i;
		*pte = (*pte)&(~PTE_A);
	}
	va += PGSIZE;
	pagetable = p->pagetable;
  }
  //printf("%p\n", ans);
  if(copyout(p->pagetable, u_addr, (char *)&ans, sizeof(ans)) < 0)
	  return -1;

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


