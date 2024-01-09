#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

// initialised priorityqueues
struct queue priorityqueue[4];

int nextpid = 1;
struct spinlock pid_lock;
extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// queue functions
int max(int a, int b)
{
  if (a > b)
  {
    return a;
  }
  return b;
}

int min(int a, int b)
{
  if (a > b)
  {
    return b;
  }
  return a;
}

int proc_rbi(struct proc *p)
{
  int a = ((3 * p->mrtime - p->stime - p->wtime) / (p->mrtime + p->wtime + p->stime + 1)) * 50;
  return max(a, 0);
}

int proc_dp(struct proc *p)
{
  return min(100, p->stime + p->RBI);
}


void pop(struct queue *h)
{
  if (h->noofelements == 0)
  {
    return;
  }
  struct queue *temp = h->last->back;
  temp->next = 0;
  // h->last->back=NULL;
  h->last = temp;
  h->noofelements--;
  return;
}

void push(struct queue *h, Procarr m)
{
  struct queue *r = (struct queue *)kalloc();
  if(r==0){
    printf("Kalloc failed\n");
  }
  r->process = m;
  r->next = 0;
  r->process->inqueuecond = 1;
  if (h->noofelements == 0)
  {
    h->next = r;
    h->noofelements++;
    h->last = r;
    r->back = h;
  }
  else
  {
    h->last->next = r;
    r->back = h->last;
    h->last = r;
    h->noofelements++;
  }
  return;
}

void push_front(Queue h, Procarr m)
{
  struct queue *r = (struct queue *)kalloc();
  if(r==0){
    printf("Kalloc failed\n");
  }
  r->process = m;
  r->process->inqueuecond = 1;
  if (h->noofelements == 0)
  {
    h->next = r;
    h->noofelements++;
    h->last = r;
    r->back = h;
    r->next = 0;
  }
  else
  {
    Queue nxt = h->next;
    h->next = r;
    r->back = h;
    r->next = nxt;
    nxt->back = r;
    h->noofelements++;
  }
  return;
}

void printlast(struct queue *h)
{
  if (h->noofelements != 0)
  {
    printf("last element: %d ", h->last->process->pid);
  }
}

void printqueue(struct queue *h)
{
  Queue temp = h->next;
  while (h->noofelements != 0 && temp != h->last->next)
  {
    printf("%d  ", temp->process->pid);
    temp = temp->next;
  }
  return;
}
// Procarr firstelement(struct queue*h){
//   if(h->lastindex==0){
//     return 0;
//   }
//   return h->processes[0];
// }

int empty(struct queue *h)
{
  if (h->noofelements == 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void remove(struct queue *h, uint64 pid)
{
  Queue temp = h->next;
  while (temp != 0 && h->noofelements != 0)
  {
    if (temp->process->pid == pid)
    {
      temp->process->inqueuecond = 0;
      if (temp->next == 0)
      {
        Queue w = temp->back;
        w->next = 0;
        h->last = w;
        h->noofelements--;
        kfree((void *)temp);
      }
      else
      {
        Queue w1 = temp->back;
        Queue w2 = temp->next;
        w1->next = w2;
        w2->back = w1;
        // h->last=w2;
        h->noofelements--;
        kfree((void *)temp);
      }
      // temp->process=0;

      break;
    }
    temp = temp->next;
  }

  return;
}

int checkrunnable(struct queue *h)
{
  int c = 0;
  Queue temp = h->next;
  while (temp != 0 && h->noofelements != 0)
  {
    if (temp->process->state == RUNNABLE)
    {
      c++;
      break;
    }
    temp = temp->next;
  }
  return c;
}

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void procinit(void)
{
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == UNUSED)
    {
      goto found;
    }
    else
    {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->alarmcond = 0;
  p->handleradd = 0;
  p->intervalticks = 0;
  p->nticks = 0;
  p->pid = allocpid();
  p->state = USED;
  p->inqueuecond = 1;
  p->cpurtime = 0;
  p->queue_num = 0;
  p->queuetimeslice[0] = 1;
  p->queuetimeslice[1] = 3;
  p->queuetimeslice[2] = 9;
  p->queuetimeslice[3] = 15;
  p->queuewaittime = ticks;
  // Allocate a trapframe page.
  if ((p->trapframe = (struct trapframe *)kalloc()) == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;
  p->rtime = 0;
  p->etime = 0;
  p->ctime = ticks;
  #ifdef MLFQ
  push(&priorityqueue[0], p);
  #endif
  // printf("Process entered queue:%d\n",p->pid);
  // printlast(&priorityqueue[p->queue_num]);
  // printf("\n");
  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  #ifdef MLFQ
  remove(&priorityqueue[p->queue_num], p->pid);
  #endif
  // p->inqueuecond=0;
  // printf("removing\n");
  if (p->trapframe)
    kfree((void *)p->trapframe);
  if (p->copytrap)
  {
    kfree((void *)p->copytrap);
  }
  p->trapframe = 0;
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);

  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE,
               (uint64)trampoline, PTE_R | PTE_X) < 0)
  {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE,
               (uint64)(p->trapframe), PTE_R | PTE_W) < 0)
  {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
    0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
    0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
    0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
    0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;     // user program counter
  p->trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0)
  {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0)
    {
      return -1;
    }
  }
  else if (n < 0)
  {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
  {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p)
{
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++)
  {
    if (pp->parent == p)
    {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status)
{
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd])
    {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;
  p->etime = ticks;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++)
    {
      if (pp->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if (pp->state == ZOMBIE)
        {
          // Found one.
          pid = pp->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                   sizeof(pp->xstate)) < 0)
          {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p))
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void)
{
  struct cpu *c = mycpu();

  c->proc = 0;
  for (;;)
  {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

#ifdef RR
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
#endif

#ifdef FCFS
    struct proc *p;
    struct proc *min_process = 0;
    for (p = proc; p < &proc[NPROC]; p++)
    {

      acquire(&p->lock);
      // int flag=1;
      if (p->state == RUNNABLE)
      {
        if (min_process == 0)
        {
          min_process = p;
          // flag=0;
        }
        else
        {
          if (p->ctime < min_process->ctime)
          {
            // release(&min_process->lock);
            min_process = p;
            // flag=0;
          }
        }
      }

      // if(flag){
      release(&p->lock);
      // }
    }

    if (min_process != 0)
    {
      acquire(&min_process->lock);
      if (min_process->state == RUNNABLE)
      {
        min_process->state = RUNNING;
        c->proc = min_process;
        swtch(&c->context, &min_process->context);
        c->proc = 0;
      }
      release(&min_process->lock);
    }
#endif

#ifdef MLFQ
    struct proc *temp;
    for (temp = proc; temp < &proc[NPROC]; temp++)
    {
      acquire(&temp->lock);
      if (temp->inqueuecond == 0 && temp->state == RUNNABLE)
      {
        temp->inqueuecond = 1;
        temp->queuewaittime = ticks;
        push(&priorityqueue[temp->queue_num], temp);
      }
      release(&temp->lock);
    }

    for (temp = proc; temp < &proc[NPROC]; temp++)
    {
      acquire(&temp->lock);
      if (temp->inqueuecond == 1 && temp->state == RUNNABLE && (ticks - temp->queuewaittime) >= 30)
      {
        remove(&priorityqueue[temp->queue_num], temp->pid);
        if (temp->queue_num > 0)
        {
          temp->queue_num--;
        }
        temp->cpurtime = 0;
        temp->queuewaittime = ticks;
        push(&priorityqueue[temp->queue_num], temp);
      }
      release(&temp->lock);
    }

    struct proc *run_proc = 0;
    Queue NXT;
    int flag = 0;
    for (int j = 0; j < 4; j++)
    {
      Queue queueelem = priorityqueue[j].next;
      while (queueelem != 0 && priorityqueue[j].noofelements != 0)
      {
        NXT = queueelem->next;
        if (queueelem->process != 0)
        {
          acquire(&queueelem->process->lock);
          if (queueelem->process->state == RUNNABLE)
          {
            run_proc = queueelem->process;
            release(&run_proc->lock);
            remove(&priorityqueue[j], queueelem->process->pid);
            flag = 1;
            break;
          }
          else
          {
            release(&queueelem->process->lock);
            remove(&priorityqueue[j], queueelem->process->pid);
          }
        }
        queueelem = NXT;
      }
      if (flag == 1)
      {
        break;
      }
    }

    if (run_proc != 0 && flag == 1)
    {
      acquire(&run_proc->lock);
      if (run_proc->state == RUNNABLE)
      {
        run_proc->state = RUNNING;
        c->proc = run_proc;
        // run_proc->queuewaittime=ticks;
        // procdump();
        swtch(&c->context, &run_proc->context);
        c->proc = 0;
      }
      release(&run_proc->lock);
    }
#endif

#ifdef PBS
     struct proc *mp = 0;
    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->setprior == 0)
      {
        p->RBI = proc_rbi(p);
        p->DP = proc_dp(p);
      }
      else
      {
        p->setprior = 0;
      }
      if (p->state == RUNNABLE)
      {
        if (mp == 0)
        {
          mp = p;
        }
        else
        {

          if (mp->DP > p->DP)
          {
            mp = p;
          }
          else if (mp->DP < p->DP)
          {
            release(&p->lock);
            continue;
          }

          if (mp->num_schedules > p->num_schedules)
          {
            mp = p;
          }
          else if (mp->num_schedules < p->num_schedules)
          {
            release(&p->lock);
            continue;
          }

          if (mp->ctime < p->ctime)
          {
            mp = p;
          }
        }
      }
      release(&p->lock);
    }

    if (mp != 0)
    {
      acquire(&mp->lock);
      if (mp->state == RUNNABLE)
      {
        mp->state = RUNNING;
        mp->mrtime = 0;
        mp->stime = 0;
        mp->num_schedules++;
        c->proc = mp;
        swtch(&c->context, &mp->context);
        c->proc = 0;
      }
      release(&mp->lock);
    }
#endif
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&p->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
#ifdef MLFQ
  p->queuewaittime = ticks;
#endif
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); // DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p != myproc())
    {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan)
      {
        p->state = RUNNABLE;
#ifdef MLFQ
        if (p->inqueuecond == 0)
        {
          p->inqueuecond = 1;
          p->cpurtime = 0;
          p->queuewaittime = ticks;
          push(&priorityqueue[p->queue_num], p);
        }
#endif
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->pid == pid)
    {
      p->killed = 1;
      if (p->state == SLEEPING)
      {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int killed(struct proc *p)
{
  int k;

  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if (user_dst)
  {
    return copyout(p->pagetable, dst, src, len);
  }
  else
  {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if (user_src)
  {
    return copyin(p->pagetable, dst, src, len);
  }
  else
  {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [USED] "used",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  struct proc *p;
  char *state;

  printf("\n");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

#ifdef RR
    printf("%d %s %s\n", p->pid, state, p->name);
#endif

#ifdef FCFS
    printf("%d %s %s\n", p->pid, state, p->name);
#endif

#ifdef MLFQ
    char *tt = state;
    state = tt;
    // if (p->state == RUNNING)
    printf("%d %d %s %s %d %d %d\n", ticks, p->pid, state, p->name, p->queue_num, p->cpurtime, ticks - p->queuewaittime);
#endif
    // printqueue(&priorityqueue[p->queue_num]);
    //  printf("\n");
  }
  // printf("\n");
  // for (int i = 0; i < 4; i++)
  // {
  //   printf("Queue%d: ", i);
  //   printqueue(&priorityqueue[i]);
  //   // printf(" ");
  //   // printlast(&priorityqueue[i]);
  //   printf("\n");
  // }
  // printf("\n");
}

// waitx
int waitx(uint64 addr, uint *wtime, uint *rtime)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (np = proc; np < &proc[NPROC]; np++)
    {
      if (np->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if (np->state == ZOMBIE)
        {
          // Found one.
          pid = np->pid;
          *rtime = np->rtime;
          *wtime = np->etime - np->ctime - np->rtime;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                   sizeof(np->xstate)) < 0)
          {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || p->killed)
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

void update_time()
{
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == RUNNING)
    {
      p->rtime++;
#ifdef MLFQ
      p->cpurtime++;
#endif
      // if(p->pid>=9 && p->pid <= 13){
      // printf("%d %d %d\n",p->pid,p->queue_num,p->cpurtime);
      // }
    }
    release(&p->lock);
  }
  // for (p = proc; p < &proc[NPROC]; p++)
  // {
  //   if (p->state == RUNNABLE || p->state == RUNNING)
  //   {
  //     if (p->pid >= 9 && p->pid <= 13)
  //     {
  //       printf("%d %d %d\n", p->pid, ticks, p->queue_num);
  //     }
  //   }
  // }
}
