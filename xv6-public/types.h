typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
typedef int thread_t;
typedef struct xem xem_t;
typedef struct rwlock rwlock_t;
#define QUEUESIZE 1000
#define SIZE 9991

typedef struct queue{
    int q[QUEUESIZE+1];
    int last;
    int first;
    int count;
}queue;


/*
typedef struct xem_t {
       int value;
       int end;      
       struct spinlock lock;
       struct queue *q;
       int arr[SIZE];
       int count;
       int dcount;
   }xem_t;
   
    typedef struct rwlock_t{
       xem_t lock;
      xem_t writelock;
      int readers;
  }rwlock_t;

  */

