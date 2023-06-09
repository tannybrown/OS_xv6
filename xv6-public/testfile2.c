#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

#define NUM_THREAD       10
#define FILESIZE         (16000*1024)
#define BUFSIZE          256
#define FSIZE_PER_THREAD ((FILESIZE) / (NUM_THREAD))

char *filepath = "ptestfile";
int fd; 

void pwritetest();
void preadtest();

int main(int argc, char *argv[])
{
  printf(1, "pwrite test\n");
  pwritetest();
  printf(1, "Finished\n");
  
  // pread test
  printf(1, "pread test\n");
  preadtest();
  printf(1, "Finished\n");

  exit();

}

void*
pwritetestf(void *arg)
{
  int tid = (int) arg;
  int r, i, off;
  char data[BUFSIZE];

  int start = FSIZE_PER_THREAD * tid;
  int end = start + FSIZE_PER_THREAD;

  for(i = 0; i < BUFSIZE; i++)
    data[i] = (tid + i) % 128;

  printf(1, "Thread #%d is writing (%d ~ %d)\n", tid, start, end);

  for(off = start; off < end; off+=BUFSIZE){
    if ((off / BUFSIZE) % 300 == 0){
      printf(1, "Thread %d: %d bytes written\n", tid, off - start);
    }
    if ((r = pwrite(fd, data, sizeof(data), off)) != sizeof(data)){
      printf(1, "pwrite returned %d : failed\n", r);
      exit();
    }
  }

  printf(1, "Thread %d: writing finished\n", tid);
  thread_exit((void *)0);
  return 0;
}

void
pwritetest()
{
  thread_t threads[NUM_THREAD];
  int i;
  void* retval;

  fd = open(filepath, O_CREATE | O_RDWR);

  for(i = 0; i < NUM_THREAD; i++){
    if(thread_create(&threads[i], pwritetestf, (void*)i) != 0){
      printf(1, "panic at thread_create\n");
      close(fd);
      return;
    }
  }

  for (i = 0; i < NUM_THREAD; i++){
    if (thread_join(threads[i], &retval) != 0){
      printf(1, "panic at thread_join\n");
      close(fd);
      return;
    }
  }
  close(fd);
}

void*
preadtestf(void *arg)
{
  int tid = (int) arg;
  int r, off, i;
  char buf[BUFSIZE];

  int start = FSIZE_PER_THREAD * tid;
  int end = start + FSIZE_PER_THREAD;

  printf(1, "Thread #%d is reading (%d ~ %d)\n", tid, start, end);

  for(off = start; off < end; off+=BUFSIZE){
    if ((r = pread(fd, buf, sizeof(buf), off)) != sizeof(buf)){
      printf(1, "pread returned %d : failed\n", r);
      exit();
    }
    for (i = 0; i < BUFSIZE; i++) {
      if (buf[i] != (tid + i) % 128) {
        printf(1, "data inconsistency detected\n");
        exit();
      }
    }
  }
  thread_exit((void *)0);
    return 0;
}

void
preadtest()
{
  thread_t threads[NUM_THREAD];
  int i;
  void* retval;

  fd = open(filepath, O_RDONLY);

  for(i = 0; i < NUM_THREAD; i++){
    if(thread_create(&threads[i], preadtestf, (void*)i) != 0){
      printf(1, "panic at thread_create\n");
      close(fd);
      return;
    }
  }

  for (i = 0; i < NUM_THREAD; i++){
    if (thread_join(threads[i], &retval) != 0){
      printf(1, "panic at thread_join\n");
      close(fd);
      return;
    }
  }
  close(fd);
}
