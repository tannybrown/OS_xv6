//for thread
typedef int thread_t;

// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

//priorityBoost
void priorityBoost(void);
//thread yield
int thread_yield(struct proc *p);

int create_thread(struct proc *p);

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)

  //for MLFQ
  int qLevel;                  // level of queue in mlfq, it may be -1~2(if qLevel==-1,it is out of mlfq.) 
  uint runTime;                // time that process run  
  int round;                   // to check time quantum in Middle queue
  int quantumtick;             // to check time quantum
//  int roundLow;                // to check time quantum in Lowest queue
  //for Stride
  int checkMLFQ;              // check whether current process is mlfq or not(0=mlfq,0=!stride) 
  float pass;                   //pass value of this process(stride)
  float stride;                 //stride
  int myPortion;                 //cpu portion that this process has
  //for process
  int havet;                //if 0 => have no thread, 1 = have thread
  //for thread
  struct proc *main_thread;    //main thread 
  int t_flag;                   //if thread 1,if process 0
  thread_t tid;                 //thread id
  void *retval;                 //return value
  int ptid;                   //parent's pid of thread 
  uint start;                   //thread stack memory start
  uint end;                     //thread stack memory end
  int thread_count;

  int check_count;
  int indexing;
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

