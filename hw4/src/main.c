#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <readline/readline.h>
#include <signal.h>

#include "sfish.h"
#include "debug.h"

/*typedef struct ExecStruct ExecStruct;

struct ExecStruct{
    char *execpath;
    char **vargs;
    char *redirectout = NULL;
    int outpos = -1;
    char *redirectin = NULL;
    int inpos = -1;
};*/

sigset_t mask, prev;
char currentDir[256];
char prevDir[256] = {0};
pid_t childpid;
pid_t pid;
char *tokptr;
int pipefd1[2] = {-1, -1};
int pipefd2[2] = {-1 ,-1};
char *outpos;
char *inpos;

int execute(const char *pathname, char *vargs[]);
int prepexecute(char *execpath, char *currarg, char *inputptr);
int findexecutable(char *input);
int checkredirection(char *currarg);
char *trimwhitespace(char *token);
void exitfork(int success, int inputfd, int outputfd);

void sigchld_handler(int s)
{
    int olderrno = errno;
    pid = waitpid(childpid, NULL, 0);
    errno = olderrno;
}
void sigint_handler(int s)
{
}

int main(int argc, char *argv[], char* envp[]) {
    signal(SIGCHLD, sigchld_handler);
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    //sigaddset(&mask, SIGINT);
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
            strcat(strcat(prompt, homeDir + strlen(home)), " :: jerchu >> ");
        }
        else
            strcat(strcat(prompt, currentDir), " :: jerchu >> ");
        input = readline(prompt);

        write(1, "\e[s", strlen("\e[s"));
        write(1, "\e[20;10H", strlen("\e[20;10H"));
        write(1, "SomeText", strlen("SomeText"));
        write(1, "\e[u", strlen("\e[u"));

        // If EOF is read (aka ^D) readline returns NULL
        if(input == NULL) {
            printf("\n");
            continue;
        }
        else if(strlen(input) == 0){
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
                    printf(BUILTIN_ERROR, "cd: No previous directory");
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
                        char errstr[1000] = {0};
                        sprintf(errstr, "cd: there was a problem going to directory %s\n", newdir);
                        printf(BUILTIN_ERROR, errstr);
                    }
                }
                //printf(EXEC_NOT_FOUND, input);
            }
        }
        else if(strstr(input, "pwd") == input){
            printf("%s\n", currentDir);
        }
        else if(!exited){
            char *token;
            //ExecStruct **exec = calloc(1, sizeof(ExecStruct))
            token = strtok_r(input, "|", &tokptr);
            token = trimwhitespace(token);
            findexecutable(token);
        }

        // Readline mallocs the space for input. You must free it.
        rl_free(input);

    } while(!exited);

    debug("%s", "user entered 'exit'");

    return EXIT_SUCCESS;
}

char *trimwhitespace(char *token){
    while(*token == ' ' || *token == '\t'){
        *token = 0;
        token++;
    }
    //debug("%s", token);
    int end = strlen(token)-1;
    while(*(token+end) == ' ' || *(token+end) == '\t'){
        *(token+end) = 0;
        end--;
    }
    //debug("%s", token);
    return token;
}

int findexecutable(char *input){
    //ArgStruct *args;
    outpos = strstr(input, ">");
    inpos = strstr(input, "<");
    if(outpos > 0){
        *outpos = 0;
        outpos++;
    }
    if(inpos > 0){
        *inpos = 0;
        inpos++;
    }
    char *inputptr;
    char *currarg = strtok_r(input, " ", &inputptr);
    //args->nextarg = NULL;
    struct stat stt;
    if(currarg != NULL && strstr(currarg, "/") != NULL){
        debug("trying relative/absolute path %d", 0);
        int success = stat(currarg, &stt);
        if(!success){
            prepexecute(currarg, currarg, inputptr);
        }
    }
    else if(currarg != NULL){
        char *path = malloc(strlen(getenv("PATH")));
        strcpy(path, getenv("PATH"));
        //debug("%s", path);
        char *pathptr;
        char *pathtok = strtok_r(path, ":", &pathptr);
        //debug("%s", pathtok);
        char execpath[256] = {0};
        //debug("%s", pathtok);
        strcat(strcat(strcat(execpath, pathtok), "/"), currarg);
        //debug("current exec path: %s\n", execpath);
        int success = stat(execpath, &stt);
        //debug("success: %d %d\n", success, errno);
        while(success < 0 && pathtok != NULL){
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
            prepexecute(execpath, currarg, inputptr);
        }
        else{
            printf(EXEC_NOT_FOUND, input);
        }
        free(path);
    }
    else{
        debug("printing else case %d", 0);
        printf(EXEC_NOT_FOUND, input);
    }
    return EXIT_SUCCESS;
}

int prepexecute(char *execpath, char *currarg, char *inputptr){
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
    int error = execute(execpath, argv);
    memset(pipefd1, -1, sizeof(int) * 2);
    memset(pipefd2, -1, sizeof(int) * 2);
    error = error;
    debug("did the thing %d\n", error);
    free(argv);
    return EXIT_SUCCESS;
}

int checkredirection(char *currarg)
{
    if(strstr(currarg, "<")==0){
        return INPUT_REDIRECTION;
    }
    else if(strcmp(currarg, ">")==0){
        return OUTPUT_REDIRECTION;
    }
    else if(strcmp(currarg, "<")==0){
        return PIPING_REDIRECTION;
    }
    else{
        return NO_REDIRECTION;
    }
}

int execute(const char *pathname, char *vargs[]){
    sigprocmask(SIG_BLOCK, &mask, &prev);
    pid = 0;
    char *token = strtok_r(NULL, "|", &tokptr);
    debug("%d, %d", pipefd1[0], pipefd1[1]);
    if(token != NULL){
        pipe(pipefd2);
        debug("%d, %d", pipefd2[0], pipefd2[1]);
    }
    if((childpid = fork()) == 0){
        sigprocmask(SIG_SETMASK, &prev, NULL);
        //debug("%d", redirection);
        int inputfd = pipefd1[0];
        int outputfd = pipefd2[1];
        //debug("%d", redirection);
        debug("doing this? %ld", (long)outpos);
        if(inpos > 0 && (inpos < outpos || !outpos) && inputfd < 0){
            debug("%s",inpos);
            inpos = trimwhitespace(inpos);
            debug("%s",inpos);
            debug("%zd", strlen(inpos));
            inputfd = open(inpos, O_RDONLY);
            if(inputfd < 0){
                debug("%d", inputfd);
                printf(SYNTAX_ERROR, "invalid file name");
                exitfork(EXIT_FAILURE, inputfd, outputfd);
            }
        }
        if(outpos > 0 && outputfd < 0){
            debug("%s", outpos);
            outpos = trimwhitespace(outpos);
            debug("%s", outpos);
            outputfd = open(outpos, O_RDWR | O_CREAT | O_TRUNC, 0666);
            if(outputfd < 0){
                debug("%d", errno);
                printf(SYNTAX_ERROR, "invalid file name");
                exitfork(EXIT_FAILURE, inputfd, outputfd);
            }
        }
        //debug("%d", redirection);
        if(inputfd >= 0){
            debug("duping %d", inputfd);
            dup2(inputfd, STDIN_FILENO);
        }
        if(outputfd >= 0){
            debug("duping %d", outputfd);
            dup2(outputfd, STDOUT_FILENO);
        }
        //close(pipefd1[0]);
        //close(pipefd1[1]);
        //close(pipefd2[0]);
        //close(pipefd2[1]);
        char execpath[256] = {0};
        if(strstr(pathname, "/") == pathname)
            strcat(execpath, pathname);
        else
            strcat(strcat(strcat(execpath, currentDir), "/"), pathname);
        int error = execv(pathname, vargs);
        debug("%s", "program executed");
        if(error == -1){
            debug("%d, %d\n", error, errno);
            exitfork(EXIT_FAILURE, inputfd, outputfd);
        }

        if(inpos > 0 && inpos > outpos && inputfd < 0){
            inpos = trimwhitespace(inpos);
            inputfd = open(inpos, O_RDONLY);
            dup2(inputfd, STDIN_FILENO);
            if(inputfd < 0){
                //debug("%d", redirection);
                printf(SYNTAX_ERROR, "invalid file name");
                exit(EXIT_FAILURE);
            }
            else{
                char c;
                while((c = getchar()) != EOF)
                    putchar(c);
            }
        }
        exitfork(EXIT_SUCCESS, inputfd, outputfd);
    }
    else{
        signal(SIGINT, sigint_handler);
    }
    //close(pipefd1[0]);
    //close(pipefd1[1]);
    //close(pipefd2[0]);
    close(pipefd2[1]);
    while(pid != childpid)
        sigsuspend(&prev);
    //for now just reap the child
    sigprocmask(SIG_SETMASK, &prev, NULL);
    signal(SIGINT, SIG_DFL);
    memcpy(pipefd1, pipefd2, sizeof(int) * 2);
    memset(pipefd2, -1, sizeof(int) * 2);
    if(token != NULL){
        token = trimwhitespace(token);
        findexecutable(token);
    }
    //wait(NULL);
    return EXIT_SUCCESS;
}

void exitfork(int success, int inputfd, int outputfd){
    if(inputfd>=0)
        close(inputfd);
    if(outputfd>=0){
        debug("%s", "output closed");
        close(outputfd);
    }
    exit(success);
}
