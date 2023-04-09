#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

#define BUFSIZE (512)
#define STRESSCNT (4)

int main(int argc, char *argv[]) {
  int fd; 
  int i, j, k;
  char c[BUFSIZE];
  char r[BUFSIZE];
  struct stat st1;
  struct stat stt[STRESSCNT * 2];

  for (i = 0; i < BUFSIZE; i++) {
    c[i] = 'a' + (i % 26);
  }

  printf(1, "create test\n");
  fd = open("testfile", O_CREATE | O_RDWR);
  for (i = 0; i < 1024 * 16 * 2; i++) {
  //  printf(1,"a %d\n",i);
    if (write(fd, c, BUFSIZE) == -1) {
      printf(1, "error: write failed at %d\n", i);
      exit();
    }
  }
  close(fd);
  k = i * BUFSIZE;
  printf(1, "total write: %d\n", k);
  stat("testfile", &st1);
  printf(1, "file size: %d\n", st1.size);
  printf(1, "create test passed\n");
  

  printf(1, "read test\n");
  fd = open("testfile", O_RDONLY);
  for (i = 0; i < 1024 * 16 * 2; i++) {
    if (read(fd, r, BUFSIZE) == -1) {
      printf(1, "error: read failed\n");
      exit();
    }
    for (j = 0; j < BUFSIZE; j++) {
      if (r[j] != c[j]) {
        printf(1, "error: wrong write at %d %c\n", i, *r);
        exit();
      }
    }
  }
  close(fd);
  k = i * BUFSIZE;
  printf(1, "total read: %d\n", k);
  printf(1, "read test passed\n");

  if (unlink("testfile") == -1) {
    printf(1, "error: delete file failed\n");
    exit();
  }

  printf(1, "stress test\n");
  for (i = 0; i < 4; i++) {
    fd = open("stressfile", O_CREATE | O_RDWR);
    printf(1, "stressfile created\n");
    for (j = 0; j < 1024 * 16 * 2; j++) {
      if (write(fd, c, BUFSIZE) == -1) {
	printf(1, "error: write failed at %d\n", i);
	exit();
      }
    }
    close(fd);
    k = j * BUFSIZE;
    printf(1, "total write: %d\n", k);
    stat("stressfile", &stt[i*2]);
    printf(1, "stressfile size: %d\n", stt[i*2].size);
    if (unlink("stressfile") == -1) {
      printf(1, "error: delete file failed\n");
      exit();
    }
    printf(1, "stress file deleted\n");
    stat("stressfile", &stt[i*2+1]);
    printf(1, "stressfile size: %d\n", stt[i*2+1]);
  }
  printf(1, "stress test passed\n");
  exit();
}

