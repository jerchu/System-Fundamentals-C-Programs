#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <readline/readline.h>
#include <signal.h>

#include "sfish.h"
#include "debug.h"

typedef struct ArgStruct ArgStruct;

struct ArgStruct{
    char *currarg;
    ArgStruct *nextarg;
};

sigset_t mask, prev;
char currentDir[256];
char prevDir[256] = {0};

int execute(const char *pathname, char *vargs[]);
int prepexecute(char *currarg, char *inputptr);

void sigchld_handler(int s)
{
    int olderrno = errno;
    pid_t pid = wait(NULL);
    pid = pid;
    errno = olderrno;
}

int main(int argc, char *argv[], char* envp[]) {
    signal(SIGCHLD, sigchld_handler);
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    char* input;
    bool exited = false;

    if(!isatty(STDIN_FILENO)) {
        // If your shell is reading from a piped file
        // Don't have readline write anything to that file.
        // Such as the prompt or "user input"
        if((rl_outstream = fopen("/dev/null", "w")) == NULL){
            perror("Failed trying to open DEVNULL");
            exit(EXIT_FAILURE);
        }
    }
    char prompt[256] = {0};
    getcwd(currentDir, 256 - strlen(" :: jerchu >> "));
    char *home = getenv("HOME");

    do {
        memset(prompt, 0, 256);
        char *homeDir = strstr(currentDir, home);
        //printf("%s\n", currentDir);
        if(homeDir){
            sprintf(prompt, "~");
            strcat(strcat(prompt, homeDir + strlen(home)), " :: jerchu >>");
        }
        else
            strcat(strcat(prompt, currentDir), " :: jerchu >>");
        input = readline(prompt);

        write(1, "\e[s", strlen("\e[s"));
        write(1, "\e[20;10H", strlen("\e[20;10H"));
        write(1, "SomeText", strlen("SomeText"));
        write(1, "\e[u", strlen("\e[u"));

        // If EOF is read (aka ^D) readline returns NULL
        if(input == NULL) {
            continue;
        }


        // Currently nothing is implemented
        //printf(EXEC_NOT_FOUND, input);

        // You should change exit to a "builtin" for your hw.
        exited = strcmp(input, "exit") == 0;
        if(strcmp(input, "help") == 0){
            printf("Available Commands:\n"
                "help: prints this helpful menu :)\n"
                "cd [dir]: changes the current working directory\n"
                "pwd: prints the current working directory\n"
                "exit: closes the shell\n");
        }
        else if(strstr(input, "cd") != NULL) {
            char *tempinput = calloc(strlen(input), 1);
            tempinput = strcpy(tempinput, input);
            char *inputptr;
            char *tokinput = strtok_r(input, " ", &inputptr);
            //if(strcmp(tokinput, "cd"))
                //printf(EXEC_NOT_FOUND, input);
            tokinput = strtok_r(NULL, " ", &inputptr);
            if(tokinput == NULL){
                memset(prevDir, 0, 256);
                strcpy(prevDir, currentDir);
                strcpy(currentDir, home);
                chdir(currentDir);
            }
            else if(strcmp(tokinput, "-") == 0){
                if(strlen(prevDir)){
                    char tempDir[256] = {0};
                    strcpy(tempDir, currentDir);
                    strcpy(currentDir, prevDir);
                    strcpy(prevDir, tempDir);
                    chdir(currentDir);
                    printf("%s\n", currentDir);
                }
                else{
                    printf("No previous directory\n");
                }
            }

            else{
                char newdir[256] = {0};
                if(tokinput[0] == '.'){
                    chdir(".");
                    if(tokinput[1] == '.'){
                        chdir("..");
                    }
                    for(;*tokinput && *tokinput != '/'; tokinput++);
                    if(*tokinput == '/'){tokinput++;}
                }
                else if(tokinput[0] == '~'){
                    chdir(home);
                    for(;*tokinput && *tokinput != '/'; tokinput++);
                    if(*tokinput == '/'){tokinput++;}
                }
                memset(prevDir, 0, 256);
                strcpy(prevDir, currentDir);
                memset(currentDir, 0, 256);
                getcwd(currentDir, 256);
                if(*tokinput){
                    //printf("%s%s\n", currentDir, tokinput);
                    //strcat(newdir, "./");
                    strcat(newdir, tokinput);
                    int success = chdir(newdir);
                    if(success > -1){
                        memset(prevDir, 0, 256);
                        strcpy(prevDir, currentDir);
                        memset(currentDir, 0, 256);
                        getcwd(currentDir, 256);
                    }
                    else{
                        printf("there was a problem going to directory %s\n", newdir);
                    }
                }
                //printf(EXEC_NOT_FOUND, input);
            }
        }
        else if(strstr(input, "pwd") == input){
            printf("%s\n", currentDir);
        }
        else if(!exited){
            //ArgStruct *args;
            char *inputptr;
            char *currarg = strtok_r(input, " ", &inputptr);
            //args->nextarg = NULL;
            struct stat stt;
            if(currarg != NULL && strstr(currarg, "/") != NULL){
                //debug("trying relative/absolute path %d", 0);
                int success = stat(currarg, &stt);
                if(!success){
                    prepexecute(currarg, inputptr);
                }
            }
            else if(currarg != NULL){
                char path[] = {0};
                strcat(path, getenv("PATH"));
                char *pathptr;
                char *pathtok = strtok_r(path, ":", &pathptr);
                char execpath[256] = {0};
                strcat(strcat(strcat(execpath, pathtok), "/"), currarg);
                //debug("current exec path: %s\n", execpath);
                int success = stat(execpath, &stt);
                //debug("success: %d %d\n", success, errno);
                while(success && pathtok != NULL){
                    pathtok = strtok_r(NULL, ":", &pathptr);
                    if(pathtok == NULL)
                        break;
                    memset(execpath, 0, 256);
                    strcat(strcat(strcat(execpath, pathtok), "/"), currarg);
                    //debug("current exec path: %s\n", execpath);
                    success = stat(execpath, &stt);
                    //debug("success: %d %d\n", success, errno);
                }
                if(!success){
                    //debug("exec path: %s\n", execpath);
                    prepexecute(execpath, inputptr);
                }
                else{
                    printf(EXEC_NOT_FOUND, input);
                }

            }
            else{
                debug("printing else case %d", 0);
                printf(EXEC_NOT_FOUND, input);
            }
        }

        // Readline mallocs the space for input. You must free it.
        rl_free(input);

    } while(!exited);

    debug("%s", "user entered 'exit'");

    return EXIT_SUCCESS;
}

int prepexecute(char *currarg, char *inputptr){
    if(strstr(currarg, "./") == currarg){
        currarg += 2;
    }
    //ArgStruct *argshead = args;
    int counter = 0;
    char **argv = malloc(sizeof(char *));
    argv[counter] = currarg;
    counter++;
    while((currarg = strtok_r(NULL, " ", &inputptr)) != NULL){
        /*ArgStruct *nextarg;
        nextarg->currarg = strtok(NULL, " ");
        nextarg->nextarg = NULL;
        args->nextarg = nextarg;
        args = nextarg;*/
        argv = realloc(argv, sizeof(char *) * (counter + 1));
        argv[counter] = currarg;
        counter++;
    }
    argv = realloc(argv, sizeof(char *) * (counter + 1));
    argv[counter] = NULL;
    execute(argv[0], argv);
    debug("did the thing %d\n", 0);
    return EXIT_SUCCESS;
}

int execute(const char *pathname, char *vargs[]){
    sigprocmask(SIG_BLOCK, &mask, &prev);
    if(fork() == 0){
        sigprocmask(SIG_SETMASK, &prev, NULL);
        char execpath[256] = {0};
        if(strstr(pathname, "/") == pathname)
            strcat(execpath, pathname);
        else
            strcat(strcat(strcat(execpath, currentDir), "/"), pathname);
        int error = execv(pathname, vargs);
        if(error == -1)
            debug("%d, %d\n", error, errno);
        exit(EXIT_SUCCESS);
    }
    sigsuspend(&prev);
    //for now just reap the child
    sigprocmask(SIG_SETMASK, &prev, NULL);
    //wait(NULL);
    return EXIT_SUCCESS;
}
