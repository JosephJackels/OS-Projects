#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"
#include <stddef.h>
#include "node.h"
#include "fcntl.h"


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct {
  struct spinlock lock;
  struct pstat ps;
} pinfo;

struct {
  long int ticks;
  struct spinlock lock;
  int used[NPROC];
  int pid[NPROC];
  int start[NPROC];
  int end[NPROC];
} ticktracker;
/*
	node {
		spinlock lock;
		int pid[];
		int used[];
		int tickets[];
	}
*/

//returns current amountof ticks that have occurred
//call with tick tracker lock
int gettotalticks(void){
  return ticktracker.ticks;
}

//returns amountof ticks used by one pid
//CALL WITH PINFO LOCK
int getpidticks(int pid){
  int tickets = -1;
  for(int i = 0; i < NPROC; i++){
    if(pinfo.ps.pid[i] == pid){
      tickets = pinfo.ps.hticks[i] + pinfo.ps.lticks[i];
      break;
    }
  }
  return tickets;
}

//CALL WITH PINFO LOCK
void printpinfo(long int ticks){
  cprintf("\nAt tick count: %d\n", ticks);
  for(int i = 0; i < NPROC; i++){
    if(pinfo.ps.inuse[i] == 1){
      cprintf("PID: %d, TICKS: %d\n", pinfo.ps.pid[i], pinfo.ps.hticks[i] + pinfo.ps.lticks[i]);
    }
  }
}
struct node hqueue, lqueue, hlqueue;//high priority queue, low priority queue, 'high' low priority queue

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

int getpinfo(struct pstat* p){
  acquire(&pinfo.lock);
  memmove(p->inuse, pinfo.ps.inuse, sizeof(pinfo.ps.inuse));
  memmove(p->pid, pinfo.ps.pid, sizeof(pinfo.ps.inuse));
  memmove(p->hticks, pinfo.ps.hticks, sizeof(pinfo.ps.hticks));
  memmove(p->lticks, pinfo.ps.lticks, sizeof(pinfo.ps.lticks));
  memmove(p->tickets, pinfo.ps.tickets, sizeof(pinfo.ps.tickets));
  memmove(p->queue, pinfo.ps.queue, sizeof(pinfo.ps.queue));
  release(&pinfo.lock);
  return 0;
}

int settickets(int id, int tickets){
  if(tickets < 1){
    //invalid ticket count!
    panic("Invalid ticket count!\n");
    return -1;
  }
  //struct proc *p = myproc();

  acquire(&pinfo.lock);
  int *i;
  ptrdiff_t index;
  for(i = pinfo.ps.pid; i < &pinfo.ps.pid[NPROC]; i++){
    if(*i == id){
      //update pinfo
      index = i - pinfo.ps.pid;
      pinfo.ps.tickets[index] = tickets;
      release(&pinfo.lock);
      acquire(&hlqueue.lock);

      //find queue and update queue
      if(isinqueue(&hlqueue, id) == 0){
        if(updatetickets(&hlqueue, id, tickets) < 0){
          release(&hlqueue.lock);
          panic("Not found in hlq? This should not happen. if it does consider adding a lock\n");
          return -1;
        }
      }
      release(&hlqueue.lock);
      acquire(&hqueue.lock);
      if(isinqueue(&hqueue, id) == 0){
          if(updatetickets(&hqueue, id, tickets) < 0){
            release(&hqueue.lock);
            panic("Not found in hq? This should not happen. if it does consider adding a lock\n");
            return -1;
          }
      }
      release(&hqueue.lock);
      acquire(&lqueue.lock);
      if(isinqueue(&lqueue, id) == 0){
          if(updatetickets(&lqueue, id, tickets) < 0){
          	release(&lqueue.lock);
            panic("Not found in lq? This should not happen. if it does consider adding a lock\n");
            return -1;
          }
      }
      release(&lqueue.lock);
      return 0;
    }
  }
  release(&pinfo.lock);
  //not found!
  panic("PID not found!\n");
  return -2;
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

void
pinfoinit(void)
{
  initlock(&pinfo.lock, "pinfo");
}

void queueinit(void)
{
	initlock(&hqueue.lock, "hqueue");
	initlock(&lqueue.lock, "lqueue");
	initlock(&hlqueue.lock, "hlqueue");
}

void ticktrackerinit(void){
  initlock(&ticktracker.lock, "ticktracker");
}
// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;
  int *i;
  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  acquire(&pinfo.lock);
  //find space in pinfostruct for proc
  ptrdiff_t index;
  for(i = pinfo.ps.inuse; i < &pinfo.ps.inuse[NPROC]; i++){
    if(*i == 0){
      index = i - pinfo.ps.inuse;
      goto found2;
    }
  }
  release(&pinfo.lock);
  return 0;

found2:
  p->pid = nextpid++;
  p->state = EMBRYO;
  pinfo.ps.inuse[index] = 1;
  pinfo.ps.pid[index] = p->pid;
  pinfo.ps.hticks[index] = 0;
  pinfo.ps.lticks[index] = 0;
  pinfo.ps.tickets[index] = 1;
  pinfo.ps.queue[index] = 1;
  
  acquire(&ticktracker.lock);
  for(i = ticktracker.used; i < &ticktracker.used[NPROC]; i++){
    if(*i == 0){
      //found a slot
      index = i - ticktracker.used;
      ticktracker.used[index] = 1;
      ticktracker.pid[index] = p->pid;
      ticktracker.start[index] = gettotalticks();
      ticktracker.end[index] = -1;
      break;
    }
  }

  acquire(&hqueue.lock);
  
  add(&hqueue, p->pid, pinfo.ps.tickets[index]);

  printpinfo(ticktracker.ticks);
  
  release(&ticktracker.lock);
  release(&hqueue.lock);
  release(&pinfo.lock);
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  
  //get ticket count from parent and set for child
  int *p;
  int parenttickets = 1;
  acquire(&pinfo.lock);
  ptrdiff_t index;
  //find parent id in table and get ticket count
  for(p = pinfo.ps.pid; p < &pinfo.ps.pid[NPROC]; p++){
    if(*p == curproc->pid){
      index = p - pinfo.ps.pid;
      parenttickets = pinfo.ps.tickets[index];
    }
  }
  //find child id in table and set ticket count
  for(p = pinfo.ps.pid; p < &pinfo.ps.pid[NPROC]; p++){
    if(*p == np->pid){
      index = p - pinfo.ps.pid;
      pinfo.ps.tickets[index] = parenttickets;
      acquire(&hlqueue.lock);
      if(isinqueue(&hlqueue, np->pid) == 0){
        if(updatetickets(&hlqueue, np->pid, parenttickets)){
          release(&hlqueue.lock);
          panic("not found in hlq\n");
        }
      }
      release(&hlqueue.lock);
      acquire(&hqueue.lock);
      if(isinqueue(&hqueue, np->pid) == 0){
        if(updatetickets(&hqueue, np->pid, parenttickets) < 0){
          release(&hqueue.lock);
          panic("Not found? This should not happen. if it does consider adding a lock\n");
        }
      }
      release(&hqueue.lock);
      acquire(&lqueue.lock);
      if(isinqueue(&lqueue, np->pid) == 0){
        if(updatetickets(&lqueue, np->pid, parenttickets) < 0){
          release(&lqueue.lock);
          panic("Not found? This should not happen. if it does consider adding a lock\n");
        }
      }
      release(&lqueue.lock);
    }
  }
  release(&pinfo.lock);

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;
  
  //set end ticks for tracker
  acquire(&ticktracker.lock);
  int pid = curproc->pid;
  for(int i = 0; i < NPROC; i++){
    if(ticktracker.pid[i] == pid){
      ticktracker.end[i] = gettotalticks();
      break;
    }
  }
  printpinfo(ticktracker.ticks);
  release(&ticktracker.lock);
  
  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  int *i;
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        acquire(&pinfo.lock);
        for(i = pinfo.ps.pid; i < &pinfo.ps.pid[NPROC]; i++){
          if(*i == pid){
            ptrdiff_t index = i - pinfo.ps.pid;
            
            acquire(&hlqueue.lock);
            if(isinqueue(&hlqueue, pid) == 0){
              if(remove(&hlqueue, pid) < 0){
              	release(&hlqueue.lock);
                panic("is in hlq but not removed?\n");
              }
            }
            release(&hlqueue.lock);
            acquire(&hqueue.lock);
            if(isinqueue(&hqueue, pid) == 0){
              if(remove(&hqueue, pid) < 0){
                release(&hqueue.lock);
                panic("is in hq but not removed?\n");
              }
            }
            release(&hqueue.lock);
            acquire(&lqueue.lock);
            if(isinqueue(&lqueue, pid) == 0){
              if(remove(&lqueue, pid) < 0){
              	release(&lqueue.lock);
                panic("is in lq but not removed?\n");
              }
            }

            release(&lqueue.lock);

            acquire(&ticktracker.lock);
            int start = 0, end = 0;
            ptrdiff_t tickindex;
            for(i = ticktracker.pid; i < &ticktracker.pid[NPROC]; i++){
              if(*i == pid){
                tickindex = i - ticktracker.pid;
                start = ticktracker.start[tickindex];
                end = ticktracker.end[tickindex]; 

                //reset index
                ticktracker.used[tickindex] = 0;
                ticktracker.start[tickindex] = 0;
                ticktracker.end[tickindex] = 0;
                ticktracker.pid[tickindex] = 0;
              }
            }
            cprintf("PID: %d started at ticks %d, ended at ticks %d, used %d of those ticks, and had a ticket priority of %d\n", pid, start, end, getpidticks(pid), pinfo.ps.tickets[index]);
            release(&ticktracker.lock);

            pinfo.ps.inuse[index] = 0;
            pinfo.ps.pid[index] = 0;
            pinfo.ps.hticks[index] = 0;
            pinfo.ps.lticks[index] = 0;
            pinfo.ps.queue[index] = 0;
            pinfo.ps.tickets[index] = 0;
          }
        }
        release(&pinfo.lock);
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

start:
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
   acquire(&ptable.lock);
    //MY SCHEDULER
    //FIRST CHOOSE FROM LHQ,
    //THEN HQ,
    //THEN LQ
    int winnerpid = -1;
    int *i;
    acquire(&pinfo.lock);
    acquire(&hlqueue.lock);
    if( (winnerpid = findwinner(&hlqueue)) > -1){
      //winner in hlq
      //increase lticks of pid by 1, move to lq
      //select pid as process to run
      for(i = pinfo.ps.pid; i < &pinfo.ps.pid[NPROC]; i++){
        if(*i == winnerpid){
          ptrdiff_t index = i - pinfo.ps.pid;
	       //cprintf("winner found in hlq. There were %d options, with a total ticket count of %d. PID %d won with a ticket count of %d\n", getused(&hlqueue), getticketsum(&hlqueue), pinfo.ps.pid[index], pinfo.ps.tickets[index]);
          //printqueue(&hlqueue, "hlqueue", winnerpid);
          pinfo.ps.lticks[index]++;
          movequeue(&hlqueue, &lqueue, winnerpid);
          break;
        }
      }
    }
    release(&hlqueue.lock);
    acquire(&hqueue.lock);
    if( (winnerpid == -1) && (winnerpid = findwinner(&hqueue)) > -1){
      //winner in hq
      //increase hticks of pid by 1, move to lq
      //select as process to run
      for(i = pinfo.ps.pid; i < &pinfo.ps.pid[NPROC]; i++){
        if(*i == winnerpid){
          ptrdiff_t index = i - pinfo.ps.pid;
          //cprintf("winner found in hq. There were %d options, with a total ticket count of %d. PID %d won with a ticket count of %d\n", getused(&hqueue), getticketsum(&hqueue), pinfo.ps.pid[index], pinfo.ps.tickets[index]);
          //printqueue(&hqueue, "hqueue", winnerpid);
          pinfo.ps.hticks[index]++;
          movequeue(&hqueue, &lqueue, winnerpid);
        }
      }
    }
    release(&hqueue.lock);
    acquire(&lqueue.lock);
    if((winnerpid == -1) && (winnerpid = findwinner(&lqueue)) > -1){
      //winner in lq 
      //increase lticks of pid by 1, move to hlq
      //select as process to run
      for(i = pinfo.ps.pid; i < &pinfo.ps.pid[NPROC]; i++){
        if(*i == winnerpid){
          ptrdiff_t index = i - pinfo.ps.pid;
          //cprintf("winner found in lq. There were %d options, with a total ticket count of %d. PID %d won with a ticket count of %d\n", getused(&lqueue), getticketsum(&lqueue), pinfo.ps.pid[index], pinfo.ps.tickets[index]);
          //printqueue(&lqueue, "lqueue", winnerpid);
          pinfo.ps.lticks[index]++;
          movequeue(&lqueue, &hlqueue, winnerpid);
        }
      }
    }
    release(&lqueue.lock);
    if(winnerpid == -1){
    	//winner wasnt found?
      release(&pinfo.lock);
      release(&ptable.lock);
      goto start;
    }
    
    release(&pinfo.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->pid == winnerpid)
        break;
    }
    
    acquire(&ticktracker.lock);
    ticktracker.ticks++;
    release(&ticktracker.lock);

    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;
    swtch(&(c->scheduler), p->context);
    switchkvm();

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
    release(&ptable.lock);
    goto start;
}

//ptable lock should be acquired before calling
int findwinner(struct node *queue){
/*randomly choose a winner from a copy of the queue. if the winning process is not runnable,
  remove it from the temporary queue and try again.
  if there are no runnable processes, or the queue is initially empty,
  return -1
  otherwise return winning process pid
  */
struct node temp;
struct proc *p;
for(int i = 0; i < NPROC; i++){
	temp.used[i] = queue->used[i];
	temp.pid[i] = queue->pid[i];
	temp.tickets[i] = queue->tickets[i];
}

tryagain:
  if(isempty(&temp) == 0){
  	//queue is initially empty, or all items in queue were not runnable and were removed from the temp queue 
    return -1;
  }

  int maxtickets = getticketsum(&temp);
  if(maxtickets == 0){
  	return -1;
  }
  int winningtickets = getrandom(maxtickets);//getrandom(0, maxtickets - 1);
  int winnerpid = -1;
  int count = 0;
  int *n;
  for(n = temp.tickets; n < &temp.tickets[NPROC]; n++){
  	ptrdiff_t index = n - temp.tickets;
  	if(temp.used[index] == 1){
  		count += temp.tickets[index];
  		if(count > winningtickets){
  			winnerpid = temp.pid[index];
  			break;
  		}
  	}
  }

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == winnerpid){
      if(p->state != RUNNABLE){
        remove(&temp, winnerpid);
        goto tryagain;
      } else {
        //printqueue(&temp, "temp - findwinner", winnerpid);
        //cprintf("Total tickets : %d, random winner range [0, %d]: %d\n\n", maxtickets, maxtickets - 1, winningtickets);
        return winnerpid;
      }
    }
  }
  return -1;
}
/* FROM https://www.cs.virginia.edu/~cr4bd/4414/F2018/lottery.html*/
static unsigned random_seed = 1;

#define RANDOM_MAX ((1u << 31u) - 1u)
unsigned lcg_parkmiller(unsigned *state)
{
    const unsigned N = 0x7fffffff;
    const unsigned G = 48271u;

    /*  
        Indirectly compute state*G%N.

        Let:
          div = state/(N/G)
          rem = state%(N/G)

        Then:
          rem + div*(N/G) == state
          rem*G + div*(N/G)*G == state*G

        Now:
          div*(N/G)*G == div*(N - N%G) === -div*(N%G)  (mod N)

        Therefore:
          rem*G - div*(N%G) === state*G  (mod N)

        Add N if necessary so that the result is between 1 and N-1.
    */
    unsigned div = *state / (N / G);  /* max : 2,147,483,646 / 44,488 = 48,271 */
    unsigned rem = *state % (N / G);  /* max : 2,147,483,646 % 44,488 = 44,487 */

    unsigned a = rem * G;        /* max : 44,487 * 48,271 = 2,147,431,977 */
    unsigned b = div * (N % G);  /* max : 48,271 * 3,399 = 164,073,129 */

    return *state = (a > b) ? (a - b) : (a + (N - b));
}

unsigned next_random() {
    return lcg_parkmiller(&random_seed);
}


int getrandom(int max){
	//returns number between 0 and max
	int rand = (int) next_random();
	return rand % (max + 1);
}
// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}