#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "xem.h"

//total portion of Stride scheduler
uint totPortion=0;
//pass value of mlfq
float passOfMlfq=0; 
//stride value of mlfq
float strideOfMlfq =0;
//process count in mlfq
int pcount=0;
//stride process count
int numStride=0;
//tid
int nexttid = 0;


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
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

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->qLevel =0;
  p->runTime = 0;
  p->checkMLFQ =0;
  p->pass=0;
  p->stride=0;
  p->myPortion=0;
  p->round = 0;
  p->quantumtick=0;
  pcount++;
  p->t_flag =0;
  p->ptid = p->pid;
  p->havet = 0;
  p->thread_count=0;
  p->check_count =0;
  p->indexing=0;
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
//  int isthread = 0;
//  int threadnum = 0;

//  if(curproc->t_flag == 1)isthread = 1;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
/*
  if(isthread ==1){
  np->sz = curproc->main_thread->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  np->havet = 1;
  }
  */
  
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

  
/*
  //for thread
  if(isthread ==1){
  for(k=ptable.proc;k<&ptable.proc[NPROC];k++){
  if(k->ptid == curproc->ptid) threadnum++;
  }
  for(int j = 0; j<threadnum;j++)
     if( create_thread(np) == -1) return -1;
  }

*/
  
  acquire(&ptable.lock);
  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

int
create_thread(struct proc *p){
struct proc *np;
struct proc *curproc = p;
// Allocate process.
    cprintf("here\n");
    if((np = allocproc()) == 0){
     return -1;
    }
    
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

    np->tf->eax = 0;
  
    for(int i = 0; i < NOFILE; i++)
      if(curproc->ofile[i])
        np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);
  
    safestrcpy(np->name, curproc->name, sizeof(curproc->name));
  
    np->main_thread = curproc;
    np->ptid = curproc->pid;
    np->state = RUNNABLE;
    return 0;
}



// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p,*k,*j;
  int fd;

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

  //process call for thread 
  for(k=ptable.proc;k<&ptable.proc[NPROC];k++){
  if(k->ptid == curproc->pid && k->state != UNUSED)
      k->state = ZOMBIE;   
  }
  //thread call for exit
  if(curproc->t_flag ==1)
  {
    for(j=ptable.proc;j<&ptable.proc[NPROC];j++){
        if(curproc->ptid == j->ptid || curproc->main_thread == j)
            j->killed = 1;
    
    }
  }


  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
    cprintf("end\n");
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p,*k;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      
        if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
          //thread free
          for(k=ptable.proc;k<&ptable.proc[NPROC];k++){
          if(k->main_thread == p)
     {
      k->kstack = 0;
      k->pid =0;
      k->parent =0;
      k->name[0]=0;
      k->killed=0;
      k->state=UNUSED;
      k->t_flag=0;
      k->tid =0;
  
      k->sz = deallocuvm(p->pgdir,p->start,p->start - 2*PGSIZE);
    
     
     
     }
          }
        // Found one.
        pid = p->pid;
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
  
  for(;;){
    // Enable interrupts on this processor.
    sti();
  
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
    //  if(p->t_flag != 0) continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
     
    }
    release(&ptable.lock);

/*
    acquire(&ptable.lock);
        
    float min = passOfMlfq;
    int High=0;
    int Mid=0;
    int Low=0;
    struct proc *highP = 0;
    struct proc *midP = 0;
    struct proc *lowP = 0;
    int sflag = 0;

   

    //if mlfq is empty
    if(numStride>0 && pcount==3)
    {passOfMlfq+=101;min+=101;}
    
    //find minimum pass value of Stride
    for(p=ptable.proc; p<&ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE)continue;
        if(p->pass<passOfMlfq && p->pass < min && p->checkMLFQ == 1)
        { min = p->pass; sflag=1; }
        if(p->qLevel == 0) High =1;
        else if(p->qLevel ==1) Mid = 1;
        else if(p->qLevel ==2) Low = 1;
    }
     
        //then, MLFQ will run and we should check qLevel. 
        //level0 exist   
        if(sflag == 0){
        if(High == 1){
        for(p=ptable.proc; p<&ptable.proc[NPROC]; p++){
            if(p->state != RUNNABLE)
                continue;
            if(p->qLevel != 0)
                continue;
            if(highP ==0)highP=p;
            else{
                if(p->round > highP->round)continue;
                if(p->round == highP->round){
                    if(p->pid < midP->pid)midP = p;}
                else highP=p;
            }
            if(highP != 0){
                p= highP;
            p->runTime++; 
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;
            passOfMlfq+=strideOfMlfq;
            swtch(&(c->scheduler),p->context);
            switchkvm();
            
            p->quantumtick++;
            if(p->quantumtick >= 5){
            p->round++;
            p->quantumtick =0;
            }
            
            if(p->runTime >= 20){
                p->qLevel =1;
                p->quantumtick=0;
                p->round=0;
            
            }
            c->proc = 0;
            }
            }
        //level 1 exist
        }else if(Mid == 1){
        for(p=ptable.proc; p<&ptable.proc[NPROC]; p++){
             if(p->state != RUNNABLE)
                 continue;
             if(p->qLevel != 1)
                 continue;
             if(midP ==0)midP= p;
             else {
                 if(p->round > midP->round)continue;
                 if(p->round == midP->round){
                 if(p->pid < midP->pid)midP =p;}
                 else midP=p;
                   } 
             if(midP != 0){
                p=midP;
                p->runTime++;
                c->proc = p;
                switchuvm(p);
                p->state = RUNNING;
                passOfMlfq+=strideOfMlfq;
  
                swtch(&(c->scheduler),p->context);
             
                switchkvm();
                p->quantumtick++;
                 if(p->quantumtick >=10)
                {
                     p->round++; 
                     p->quantumtick=0;
                }
                 if(p->runTime >= 60){
                 p->qLevel =2;
                 p->quantumtick=0;
                 p->round =0;
                 
                 }
                c->proc = 0;
             }
        //level 2(lowest queue)
        }
        }else if(Low ==1){
        for(p=ptable.proc; p<&ptable.proc[NPROC]; p++){
             if(p->state != RUNNABLE)
                 continue;
            if(lowP ==0)lowP= p;
            else{
                 if(p->round > lowP->round)continue;
                  if(p->round == lowP->round)
                  {
                    if(p->pid < lowP->pid)lowP =p;
                                                 }
                  else lowP=p;
                                                 }
                                                 }
             if(lowP!= 0){   
                p=lowP;
             p->runTime++;
             c->proc = p;
             switchuvm(p);
             p->state = RUNNING;
             passOfMlfq+=strideOfMlfq;
      
             swtch(&(c->scheduler),p->context);
             switchkvm();
             p->quantumtick++;
              if(p->quantumtick >=20)
              {
                  p->round++;
                  p->quantumtick=0;
              }
             c->proc = 0;
           }        
        }
        }
        // stride process
        else{
        
         for(p=ptable.proc; p<&ptable.proc[NPROC]; p++){
         if(p->state != RUNNABLE)continue;
         if(p->checkMLFQ == 1 && p->pass == min){
         c->proc = p;
         switchuvm(p);
         p->state = RUNNING;
         p->pass += p->stride;
         p->quantumtick++;
         swtch(&(c->scheduler),p->context);
         switchkvm();
 
         c->proc = 0;
                 }
             }
        }
    release(&ptable.lock);*/
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.

//to prevent starvation
void
priorityBoost(void){
     struct proc *p;
     acquire(&ptable.lock);
     for(p = ptable.proc; p<&ptable.proc[NPROC];p++){
     if(p->checkMLFQ == 0)
         {
            p->qLevel = 0;
            p->runTime = 0;
            p->round=0;
            p->quantumtick=0;
            
         }
     }
     release(&ptable.lock);
 }



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
int
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
  return 0;
}

int
thread_yield(struct proc* p)
{
  // int intena;
  // struct proc *p = 0;
   acquire(&ptable.lock);
   myproc()->state = RUNNABLE;  
   release(&ptable.lock);
return 0;
}


//thread create
int
thread_create(thread_t *thread,void *(*start_routine)(void *),void *arg)
{
  uint sp,sz;
  struct proc *update;
  struct proc *p;
  struct proc *np;
  uint ustack[2]; 

  p = myproc();
  sz = p->sz; 
  // for kernel stack space 
  if((np = allocproc()) == 0)return -1;
  //for thread user stack
  if((sz = allocuvm(p->pgdir,sz,sz + 2*PGSIZE)) ==0)return -1;
  clearpteu(p->pgdir,(char*)(p->sz -2*PGSIZE));  
  // for memory size update
  acquire(&ptable.lock);
  
  for (update = ptable.proc; update<&ptable.proc[NPROC]; update++){
     if(update->ptid == p->ptid) update->sz = sz;
    }
    
  release(&ptable.lock);
 
  
  acquire(&ptable.lock);
  sp = p->sz;
  np->start = sp;
  np->end = np->start - 2*PGSIZE;
  sp -= 8;

  // push pc and argument
  ustack[0] = 0xffffffff;
  ustack[1] = (uint)arg;

  if(copyout(p->pgdir,sp,ustack,8)<0) return -1;
  
  //process or thread?
  if(p->t_flag == 0) np->ptid = p->pid;
  else np->ptid = p->ptid;
  np->t_flag = 1;

  //stride or MLFQ
  //MLFQ
  if(p->checkMLFQ == 0){
  np->qLevel = p->qLevel;
  np->runTime = p->runTime;
  np->round = p->round;
  np->quantumtick = p->quantumtick;
  np->checkMLFQ = 0;
  }
  //stride
  else{
  np->pass = p->pass;
  np->stride = p->stride;
  np->myPortion = p->myPortion;
  np->runTime = p->runTime;
  np->quantumtick = p->quantumtick;
  np->checkMLFQ = 1;
  }
  
  p->havet = 1;
  np->main_thread = p;
  np->main_thread->thread_count++;
  np->tid = nexttid++;
  *thread = np->tid;
  np->pgdir = p->pgdir;
  *np->tf = *p->tf;

  np->tf->eip = (uint)start_routine;
  np->tf->esp = sp;
 
  for(update = ptable.proc; update<&ptable.proc[NPROC]; update++){
       if(update->ptid == p->ptid) update->sz = sz;
     }


  release(&ptable.lock);
  for(int i = 0; i < NOFILE; i++)
      if(p->ofile[i])
          np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);
  
  safestrcpy(np->name, p->name, sizeof(p->name));

  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);
  return 0;
}
//thread exit
void
thread_exit(void*retval)
{ 
  struct proc *p;
  struct proc *curproc = myproc();
  int fd;

  //close all open files.
  for(fd=0;fd<NOFILE; fd++){
    if(curproc->ofile[fd]){
    fileclose(curproc->ofile[fd]);
    curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd =0;

  acquire(&ptable.lock);
  //wake up main_thread, not parent!
  wakeup1(curproc->main_thread);
  
  for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
  if(p->main_thread == curproc){
    p->main_thread = initproc;
    if(p->state ==ZOMBIE)
        wakeup1(initproc);
    
    }
  }
  
  curproc->retval = retval;
  curproc->state = ZOMBIE;
  sched();

  panic("zombie exit thread");

}



//thread join
int
thread_join(thread_t thread, void **retval)
{
  struct proc *p,*k;
  int havekids;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  
  for(;;){
  havekids =0;
  for(p = ptable.proc; p<&ptable.proc[NPROC]; p++){
    if(p->main_thread != curproc||p->tid != thread)continue;
    havekids =1;
   
    if(p->state == ZOMBIE && p->tid ==thread){
    
    kfree(p->kstack);
    p->kstack = 0;    
    p->pid =0;
    p->parent =0;
    p->name[0]=0;
    p->killed=0;
    p->state=UNUSED;
    p->t_flag=0;
    p->tid =0;
    p->sz = 0;
 
    *retval = p->retval;
  
    for(k=ptable.proc;k<&ptable.proc[NPROC];k++){
            if(k->end == p->start)
            {
                k->end = p->end;
                break;
        }
        }
   
    //if this thread is located at edge of memory
    if(p->start == p->main_thread->sz){
    for (k = ptable.proc; k < &ptable.proc[NPROC]; k++) {
        if(k->state == UNUSED)continue;     
        if (k->ptid == curproc->ptid) {
                  k->sz = p->end;
              }
          }
    }

    deallocuvm(p->pgdir,p->start,p->start - 2*PGSIZE);
    p->start =0;
    p->end =0;
   release(&ptable.lock);
   return 0;
  
     
    }
  }
  
  
  if(!havekids|| curproc->killed){
  release(&ptable.lock);
  return -1;
  }
    sleep(curproc,&ptable.lock);
 
  }
  return -1;
}


// return level of q
int
getlev(void)
{
    struct proc *p;
    p=myproc();
    if(p==0)
        return -1;
    return p->qLevel;
}

 //stride value update
 void
 set_stride_value(void)
 {
     struct proc *p;
     for(p = ptable.proc; p<&ptable.proc[NPROC];p++){
     if(p->checkMLFQ == 1) p->stride = 100/(p->myPortion);
     strideOfMlfq = 100/(100- totPortion);
     }
 }
 //pass value update for preventing starvation
 void
 set_pass_value(void)
 {
     struct proc *p;
     float min = passOfMlfq;
     for(p = ptable.proc; p<&ptable.proc[NPROC];p++)
     {
     if(p->checkMLFQ == 1 && min >p->pass) min = p->pass;
     }
     passOfMlfq =((passOfMlfq) - min);
     for(p = ptable.proc; p<&ptable.proc[NPROC];p++){
     if(p->checkMLFQ == 1) {
         p->pass = p->pass - min; 
     }
     }
 }

//give portion of cpu to process
int
set_cpu_share(int portion)
{
    if(totPortion + portion >80)  return -1;
    else {
        acquire(&ptable.lock);
        numStride++;
        pcount--;
        myproc()->checkMLFQ = 1;
        myproc()->qLevel = -1;
        myproc()->quantumtick=0;
        myproc()->myPortion = portion;
        totPortion = totPortion + portion;
        set_stride_value();
        set_pass_value();

        release(&ptable.lock);
        return 0;
    }
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

void init_queue(queue *q){
    q->last = QUEUESIZE -1;
    q->first = 0;
    q->count=0;
}

int enqueue(queue *q,int x){
    q->last = (q->last +1) % QUEUESIZE;
    q->q[q->last] = x;
    q->count = q->count + 1;
    return q->last;
}

int dequeue(queue *q){
    int x;

    x = q->q[q->first];
    q->first = (q->first+1) % QUEUESIZE;
    q->count = q->count -1;

    return x;
}

int xem_init(xem_t *semaphore){
      semaphore->value = 1;
      semaphore->end = 0;
      initlock(&semaphore->lk,"");
      init_queue(semaphore->q);
      semaphore->count =0;
      semaphore->dcount =0;
      return 0;  
  }
  
  int xem_wait(xem_t *semaphore){
      acquire(&semaphore->lk);
      semaphore->value--;
      //sleep
      if(semaphore->value < 0){
   //       do{
           myproc()->chan = & semaphore->arr[semaphore->count++ % SIZE];
           sleep(myproc()->chan,&semaphore->lk);
   //       }
    //      while(semaphore->end <1);
          semaphore->end --;
      }
      release(&semaphore->lk);
      return 0;
  }
  
  int xem_unlock(xem_t *semaphore){
      acquire(&semaphore->lk);
      //wakeup
      semaphore->value++;
      if(semaphore->value <=0){
          semaphore->end++;
          wakeup(&semaphore->arr[semaphore->dcount++ % SIZE]);
      }
      release(&semaphore->lk);
      return 0;
  }
  
  int rwlock_init(rwlock_t *rwlock){
      xem_init(&rwlock->lk);
      xem_init(&rwlock->writelock);
      rwlock->readers = 0;
      return 0;
  }

  int rwlock_acquire_readlock(rwlock_t *rwlock){
      xem_wait(&rwlock->lk);
      rwlock->readers++;
      if(rwlock->readers == 1)
          xem_wait(&rwlock->writelock);
      xem_unlock(&rwlock->lk);
      return 0;
  }
  
  int rwlock_acquire_writelock(rwlock_t *rwlock){
      xem_wait(&rwlock->writelock);
      return 0;
  }
  
  int rwlock_release_readlock(rwlock_t *rwlock){
      xem_wait(&rwlock->lk);
      rwlock->readers--;
      if(rwlock->readers == 0)
          xem_unlock(&rwlock->writelock);
      xem_unlock(&rwlock->lk);
      return 0;
  }
  
  int rwlock_release_writelock(rwlock_t *rwlock){
      xem_unlock(&rwlock->writelock);
      return 0;
  }



int
  sys_xem_init(void)
  {
    xem_t *x;
    if (argptr(0, (void*)&x, sizeof(*x)) < 0)
      return -1;
    return xem_init(x);
  }
  
  int
  sys_xem_wait(void)
  {
    xem_t *x;
    if (argptr(0, (void*)&x, sizeof(*x)) < 0)
      return -1;
    return xem_wait(x);
  
  }
  
  int
  sys_xem_unlock(void)
  {
    xem_t *x;
    if (argptr(0, (void*)&x, sizeof(*x)) < 0)
      return -1;
    return xem_unlock(x);
  }
  int
  sys_rwlock_init(void)
  {
    rwlock_t *x;
    if (argptr(0, (void*)&x, sizeof(*x)) < 0)
      return -1;
    return rwlock_init(x);
  }
  
  int
 sys_rwlock_acquire_readlock(void)
 {
   rwlock_t *x;
   if (argptr(0, (void*)&x, sizeof(*x)) < 0)
     return -1;
   return rwlock_acquire_readlock(x);
 }

int
 sys_rwlock_release_readlock(void)
 {
   rwlock_t *x;
   if (argptr(0, (void*)&x, sizeof(*x)) < 0)
     return -1;
   return rwlock_release_readlock(x);
 }
 
 int
 sys_rwlock_acquire_writelock(void)
{
   rwlock_t *x;
   if (argptr(0, (void*)&x, sizeof(*x)) < 0)
     return -1;
   return rwlock_acquire_writelock(x);
 }
 
 int
 sys_rwlock_release_writelock(void)
 {
   rwlock_t *x;
   if (argptr(0, (void*)&x, sizeof(*x)) < 0)
     return -1;
   return rwlock_release_writelock(x);
 }
