// Long-term locks for processes
struct sleeplock {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
};


struct xem {
           int value;
           int end;
           struct spinlock lk;
           struct queue *q;
           int arr[SIZE];
           int count;
           int dcount;
       };
typedef struct xem xem_t;
 
 struct rwlock{
         xem_t lk;
         xem_t writelock;
         int readers;
       };
typedef struct rwlock rwlock_t;
