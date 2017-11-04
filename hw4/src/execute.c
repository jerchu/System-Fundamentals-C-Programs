#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>

#include "debug.h"

/*int execute(const char *pathname, const char *vargs[]){
    if(fork() == 0){
        execv(pathname, vargs);
        exit();
    }
    sigsuspend(SIGCHLD);
    //for now just reap the child
    wait();
}*/