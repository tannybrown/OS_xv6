#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char*argv[]){

int pid;

for(;;){
pid = fork();
if (pid>0)
    {
        printf(1,"parent\n");
        yield();
    }
else if (pid == 0){
    printf(1,"child\n");
    yield();
}
wait();
}
exit();
}
