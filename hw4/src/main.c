#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <readline/readline.h>
#include <signal.h>
#include <termios.h>
#include <time.h>

#include "sfish.h"
#include "debug.h"

typedef struct JobStruct JobStruct;

struct JobStruct{
    char *execname;
    pid_t pid;
    int stopped;
    int bgproc;
    struct termios tmodes;
    JobStruct *prev;
    JobStruct *next;
};

sigset_t mask, prev;
char currentDir[256];
char prevDir[256] = {0};
pid_t childpid;
pid_t pid;
int status;
char *tokptr;
int pipefd1[2] = {-1, -1};
int pipefd2[2] = {-1 ,-1};
char *outpos;
char *inpos;
char *color = ANSI_COLOR_RESET;
JobStruct *joblist;
struct termios shell_tmodes;

int execute(const char *pathname, char *vargs[]);
int prepexecute(char *execpath, char *currarg, char *inputptr);
int findexecutable(char *input);
int checkredirection(char *currarg);
char *trimwhitespace(char *token);
void exitfork(int success, int inputfd, int outputfd);

void sigchld_handler_shell_bg(int s){
    int olderrno = errno;
    pid = waitpid(childpid, &status, WUNTRACED | WNOHANG);
    errno = olderrno;
}

void sigchld_handler_shell(int s)
{
    int olderrno = errno;
    pid = waitpid(childpid, &status, WUNTRACED);
    JobStruct *job = joblist->next;
    for(; job!=NULL && job->pid != pid; job = job->next);
    if(!WIFSTOPPED(status)){
        if(job){
            job->prev->next = job->next;
            if(job->next)
                job->next->prev = job->prev;
            free(job);
        }
    }
    else if(job){
        job->stopped = 1;
    }
    errno = olderrno;
}
void sigchld_handler(int s){
    int olderrno = errno;
    pid = waitpid(childpid, &status, 0);
    errno = olderrno;
}
void sigint_handler(int s)
{
}

void sigtstp_handler(int s)
{
}

int main(int argc, char *argv[], char* envp[]) {
    joblist = malloc(sizeof(JobStruct));
    joblist->execname = NULL;
    joblist->prev = NULL;
    joblist->next = NULL;
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &prev);
    //sigaddset(&mask, SIGINT);
    if(getpgid(0) < 0)
        setpgid(0,0);
    tcsetpgrp(STDIN_FILENO, getpgid(0));
    tcsetpgrp(STDOUT_FILENO, getpgid(0));
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
        char colorprompt[1000] = {0};
        strcat(strcat(strcat(colorprompt, color), prompt), ANSI_COLOR_RESET);

        /*struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        //printf("%d\n", w.ws_col);
        write(1, "\e[s", strlen("\e[s"));
        write(1, "\e[0G", strlen("\e[0G"));
        char distancef[100] = {0};
        sprintf(distancef, "\e[%dC", w.ws_col);
        write(1, distancef, strlen(distancef));
        struct tm *timeinfo;
        time_t mytime;
        mytime = time(&mytime);
        timeinfo = localtime(&mytime);
        char time[100] = {0};
        strftime(time, 100, STRFTIME_RPRMT, timeinfo);
        //debug("%s", time);
        char distanceb[100] = {0};
        sprintf(distanceb, "\e[%zdD", strlen(time));
        write(1, distanceb, strlen(distanceb));
        //write(1, "\e[20;10H", strlen("\e[20;10H"));
        write(1, time, strlen(time));
        write(1, "\e[u", strlen("\e[u"));*/

        /*int readerpid;
        if((readerpid = fork()) == 0){
            setpgid(readerpid, readerpid);
            while(1){
                signal(SIGTTOU, SIG_IGN);
                tcsetpgrp(0, readerpid);
                signal(SIGTTOU, SIG_DFL);
                write(1, "\e[s", strlen("\e[s"));
                write(1, "\e[6n", strlen("\e[6n"));
                int *x = NULL;
                int *y = NULL;
                scanf("\e[%d;%dR", x, y);
                write(1, "\e[A", strlen("\e[A"));
                write(1, "\e[0G", strlen("\e[0G"));
                write(1, distancef, strlen(distancef));
                write(1, distanceb, strlen(distanceb));
                debug("%s", w.ws_col - strlen(time));
                if(w.ws_col - strlen(time) <= *x){
                    //write(1, "\e[20;10H", strlen("\e[20;10H"));
                    write(1, "\e[0K", strlen("\e[0K"));
                }
                else{
                    write(1, time, strlen(time));
                }
                write(1, "\e[u", strlen("\e[u"));
                tcsetpgrp(0, getppid());
            }
        }*/

        input = readline(colorprompt);
        //kill(readerpid, SIGKILL);

        /*write(1, "\e[s", strlen("\e[s"));
        write(1, "\e[A", strlen("\e[A"));
        write(1, "\e[0G", strlen("\e[0G"));
        write(1, distancef, strlen(distancef));
        write(1, distanceb, strlen(distanceb));
        //write(1, "\e[20;10H", strlen("\e[20;10H"));
        write(1, "\e[0K", strlen("\e[0K"));
        write(1, "\e[u", strlen("\e[u"));*/

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
        /*if(strcmp(input, "help") == 0){
            printf("Available Commands:\n"
                "help: prints this helpful menu :)\n"
                "cd [dir]: changes the current working directory\n"
                "pwd: prints the current working directory\n"
                "exit: closes the shell\n");
        }*/
/*else*/if(strstr(input, "cd") != NULL) {
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
                    //printf("%s\n", currentDir);
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
        else if(strstr(input, "jobs")>0){
            int i = 1;
            for(JobStruct *job = joblist->next; job; job = job->next, i++){
                printf("[%d] %s\n", i, job->execname);
            }
        }
        else if(strstr(input, "fg")>0){
            char *inputptr;
            strtok_r(input, " ", &inputptr);
            char *tok = strtok_r(NULL, " ", &inputptr);
            pid_t jpid;
            if(tok != NULL && tok[0] == '%' && strspn(tok+1, "0123456789") == strlen(tok+1)){
                jpid = atoi(tok+1);
                JobStruct *job = joblist->next;
                for(pid_t i = 1; i != jpid && job; job = job->next, i++);
                if(job){
                    pid = 0;
                    signal(SIGTTOU, SIG_IGN);
                    debug("%d", getpgid(0));
                    debug("%d", getpgid(job->pid));
                    debug("%d", tcgetpgrp(STDIN_FILENO));
                    tcgetattr(STDIN_FILENO, &shell_tmodes);
                    tcsetattr(STDIN_FILENO, TCSAFLUSH, &job->tmodes);
                    tcsetpgrp(STDIN_FILENO, getpgid(job->pid));
                    tcsetpgrp(STDOUT_FILENO, getpgid(job->pid));
                    debug("%d", tcgetpgrp(STDIN_FILENO));
                    //signal(SIGTSTP, SIG_IGN);
                    debug("%s", "unsuspended");
                    if(job->stopped)
                        killpg(getpgid(job->pid), SIGCONT);
                    debug("%s", "cont sent");
                    while(!pid)
                        sigsuspend(&prev);
                    debug("%s", "process complete");
                    tcsetpgrp(STDIN_FILENO, getpgid(0));
                    tcsetpgrp(STDOUT_FILENO, getpgid(0));
                    tcgetattr(STDIN_FILENO, &job->tmodes);
                    tcsetattr(STDIN_FILENO, TCSAFLUSH, &shell_tmodes);
                    signal(SIGTTOU, SIG_DFL);
                    //sigprocmask(SIG_SETMASK, &prev, NULL);
                    /*if(!WIFSTOPPED(status)){
                        job->prev->next = job->next;
                        if(job->next)
                            job->next->prev = job->prev;
                        free(job);
                    }*/
                    debug("%s", "not stuck at proc mask");
                }
                else{
                    char errstr[1000] = {0};
                    sprintf(errstr, "fg: no job %d", jpid);
                    printf(BUILTIN_ERROR, errstr);
                }
            }
            else{
                char errstr[1000] = {0};
                sprintf(errstr, "fg: %d is not a valid job", jpid);
                printf(BUILTIN_ERROR, errstr);
            }
        }
        else if(strstr(input, "kill")>0){
            char *inputptr;
            strtok_r(input, " ", &inputptr);
            char *tok = strtok_r(NULL, " ", &inputptr);
            pid_t jpid = -1;
            JobStruct *job = NULL;
            debug("%zd, %zd", strspn(tok+1, "0123456789"), strlen(tok+1));
            if(tok != NULL && tok[0] == '%'){
                if(strspn(tok+1, "0123456789") == strlen(tok+1)){
                    jpid = atoi(tok+1);
                    job = joblist->next;
                    for(pid_t i = 1; i != jpid && job; job = job->next, i++);
                    if(job)
                        jpid = job->pid;
                }
            }
            else if(tok != NULL){
                if(strspn(tok, "0123456789") == strlen(tok)){
                    jpid = atoi(tok);
                    job = joblist->next;
                    for(;jpid != job->pid && job; job = job->next);
                }
            }
            int error;
            if(job)
                error = killpg(getpgid(jpid), SIGKILL);
            else if(jpid > -1){
                error = kill(jpid, SIGKILL);
            }
            else{
                char errstr[1000] = {0};
                sprintf(errstr, "kill: there was a problem killing process %s", tok);
                printf(BUILTIN_ERROR, errstr);
            }
            if(error < 0){
                char errstr[1000] = {0};
                sprintf(errstr, "kill: there was a problem killing process %s", tok);
                printf(BUILTIN_ERROR, errstr);
            }
            else{
                if(job){
                    job->prev->next = job->next;
                    if(job->next)
                        job->next->prev = job->prev;
                    free(job);
                }
            }
        }
        else if(strstr(input, "color")>0){
            char *inputptr;
            strtok_r(input, " ", &inputptr);
            char *tok = strtok_r(NULL, " ", &inputptr);
            if(strcmp(tok, "RED")==0)
                color = ANSI_COLOR_RED;
            else if(strcmp(tok, "GRN")==0)
                color = ANSI_COLOR_GREEN;
            else if(strcmp(tok, "YEL")==0)
                color = ANSI_COLOR_YELLOW;
            else if(strcmp(tok, "BLU")==0)
                color = ANSI_COLOR_BLUE;
            else if(strcmp(tok, "MAG")==0)
                color = ANSI_COLOR_MAGENTA;
            else if(strcmp(tok, "CYN")==0)
                color = ANSI_COLOR_CYAN;
            else if(strcmp(tok, "WHT")==0)
                color = ANSI_COLOR_WHITE;
            else if(strcmp(tok, "BWN")==0)
                color = ANSI_COLOR_BROWN;
            else
                printf(BUILTIN_ERROR, "invalid color");
        }
        /*else if(strstr(input, "pwd") == input){
            printf("%s\n", currentDir);
        }*/
        else if(!exited){
            char *token;
            //ExecStruct **exec = calloc(1, sizeof(ExecStruct))
            char *name = malloc(50);
            strncpy(name, input, 47);
            if(strlen(name) > 46)
                strcat(name, "...");
            JobStruct *job = malloc(sizeof(JobStruct));
            job->execname = name;
            job->stopped = 0;
            JobStruct *jobn = joblist;
            for(; jobn->next; jobn = jobn->next);
            job->prev = jobn;
            job->next = NULL;
            jobn->next = job;
            input = trimwhitespace(input);
            int bgproc = 0;
            char *ampersand_index;
            if((ampersand_index = rindex(input, '&')) == input + strlen(input) - 1){
                bgproc = 1;
                *ampersand_index = 0;
            }
            token = strtok_r(input, "|", &tokptr);
            token = trimwhitespace(token);
            //sigprocmask(SIG_BLOCK, &mask, &prev);
            pid = 0;
            debug("%d", getpgid(0));
            if((job->pid = (childpid = fork())) == 0){
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                debug("%d", getpgid(0));

                //tcsetpgrp(STDOUT_FILENO, getpgrp());
                sigprocmask(SIG_SETMASK, &prev, NULL);
                findexecutable(token);
                exit(EXIT_SUCCESS);
            }
            signal(SIGCHLD, sigchld_handler_shell);
            //job->pid = childpid;
            //debug("%d", job->pid);
            tcgetattr(STDIN_FILENO, &shell_tmodes);
            setpgid(childpid, childpid);
            //debug("%d, %d", tcsetpgrp(STDOUT_FILENO, getpgid(childpid)), errno);
            //printf("%d", tcgetpgrp(0));
            signal(SIGTTOU, SIG_IGN);
            if(!bgproc){
                tcsetpgrp(STDIN_FILENO, getpgid(childpid));
                tcsetpgrp(STDOUT_FILENO, getpgid(childpid));
                while(childpid != pid)
                    sigsuspend(&prev);
                /*if(WIFEXITED(status)){
                    if(WEXITSTATUS(status)==EXIT_FAILURE)
                        printf(EXEC_ERROR, "process failed to execute");
                }*/
                debug("%d", tcgetpgrp(0));
                if(WIFSTOPPED(status))
                    tcgetattr(STDIN_FILENO, &job->tmodes);
                tcsetpgrp(STDIN_FILENO, getpgid(0));
                tcsetpgrp(STDOUT_FILENO, getpgid(0));
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &shell_tmodes);
                //tcsetpgrp(STDOUT_FILENO, getpgid(0));
                signal(SIGTTOU, SIG_DFL);
                //sigprocmask(SIG_SETMASK, &prev, NULL);
                /*if(!WIFSTOPPED(status)){
                    job->prev->next = job->next;
                    if(job->next)
                        job->next->prev = job->prev;
                    free(job);
                }
                else{
                    job->stopped = 1;
                }*/
            }
            else{
                tcgetattr(STDIN_FILENO, &job->tmodes);
                tcsetpgrp(STDIN_FILENO, getpgid(0));
                tcsetpgrp(STDOUT_FILENO, getpgid(0));
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &shell_tmodes);
                signal(SIGTTOU, SIG_DFL);
            }
            debug("%s", "not stuck at proc mask");
        }
        // Readline mallocs the space for input. You must free it.
        rl_free(input);

    } while(!exited);

    debug("%s", "user entered 'exit'");
    for(JobStruct *job = joblist->next; job!=NULL; job = job->next)
        killpg(getpgid(job->pid), SIGKILL);

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
    if(currarg != NULL && strcmp(currarg, "pwd") == 0){
        execute("pwd", NULL);
    }
    else if(currarg!=NULL && strcmp(currarg, "help") == 0){
        execute("help", NULL);
    }
    else if(currarg != NULL && strstr(currarg, "/") != NULL){
        debug("trying relative/absolute path %d", 0);
        int success = stat(currarg, &stt);
        if(!success){
            prepexecute(currarg, currarg, inputptr);
        }
        else{
            char errstr[1000] = {0};
            sprintf(errstr, "%s: command not found", currarg);
            printf(EXEC_ERROR, errstr);
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
            char errstr[1000] = {0};
            sprintf(errstr, "%s: command not found", currarg);
            printf(EXEC_ERROR, errstr);
        }
        free(path);
    }
    else{
        debug("printing else case %d", 0);
        char errstr[1000] = {0};
        sprintf(errstr, "%s: command not found", currarg);
        printf(EXEC_ERROR, errstr);
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
    pid = 0;
    char *token = strtok_r(NULL, "|", &tokptr);
    debug("%d, %d", pipefd1[0], pipefd1[1]);
    if(token != NULL){
        pipe(pipefd2);
        debug("%d, %d", pipefd2[0], pipefd2[1]);
    }
    if((childpid = fork()) == 0){
        sigprocmask(SIG_SETMASK, &prev, NULL);
        //setpgid(0, getpgid(STDIN_FILENO));
        //tcsetpgrp(STDIN_FILENO, getpgid(getppid()));
        //tcsetpgrp(STDOUT_FILENO, getpgid(getppid()));
        //debug("%d", redirection);
        int inputfd = pipefd1[0];
        int outputfd = pipefd2[1];
        //debug("%d", redirection);
        debug("doing this? %ld", (long)inpos);
        if(inpos > 0 && (inpos < outpos || !outpos) && inputfd < 0){
            debug("%s",inpos);
            inpos = trimwhitespace(inpos);
            debug("%s",inpos);
            debug("%zd", strlen(inpos));
            inputfd = open(inpos, O_RDONLY);
            debug("%d", inputfd);
            if(inputfd < 0){
                debug("%d", inputfd);
                printf(SYNTAX_ERROR, "invalid file name");
                exitfork(EXIT_FAILURE, inputfd, outputfd);
            }
        }
        else if(inpos > 0 && inputfd >= 0){
            printf(SYNTAX_ERROR, "redirection < cannot be used after a pipeline");
            exitfork(EXIT_FAILURE, inputfd, outputfd);
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
        else if(outpos > 0 && outputfd >= 0){
            printf(SYNTAX_ERROR, "redirection > cannot be used before a pipeline");
            exitfork(EXIT_FAILURE, inputfd, outputfd);
        }
        //debug("%d", redirection);
        if(inputfd >= 0){
            debug("duping %d", inputfd);
            dup2(inputfd, STDIN_FILENO);
            /*char c;
            while((c = getchar()) != EOF){
                debug("%c", c);
            }*/
        }
        if(outputfd >= 0){
            debug("duping %d", outputfd);
            dup2(outputfd, STDOUT_FILENO);
        }
        //close(pipefd1[0]);
        //close(pipefd1[1]);
        //close(pipefd2[0]);
        //close(pipefd2[1]);
        if(strcmp(pathname, "pwd")==0 && vargs == NULL){
            printf("%s\n", currentDir);
            exitfork(EXIT_SUCCESS, inputfd, outputfd);
        }
        else if(strcmp(pathname, "help")==0 && vargs == NULL){
            printf("Available Commands:\n"
                "help: prints this helpful menu :)\n"
                "cd [dir]: changes the current working directory\n"
                "pwd: prints the current working directory\n"
                "fg %%jid: continues the process %%jid\n"
                "kill %%jid | pid: kill the process with %%jid or pid\n"
                "exit: closes the shell\n");
            exitfork(EXIT_SUCCESS, inputfd, outputfd);
        }
        debug("%s", "built-ins");
        char execpath[256] = {0};
        if(strstr(pathname, "/") == pathname)
            strcat(execpath, pathname);
        else
            strcat(strcat(strcat(execpath, currentDir), "/"), pathname);
        debug("%s", "executing program");
        //sigprocmask(SIG_BLOCK, &mask, &prev);
        debug("%d, %d", inputfd, outputfd);
        if((childpid = fork()) == 0){
            //sigprocmask(SIG_SETMASK, &prev, NULL);
            execv(pathname, vargs);
            exit(EXIT_FAILURE);
        }
        while(childpid != pid)
            sigsuspend(&prev);
        if(WIFEXITED(status)){
            if(WEXITSTATUS(status) == EXIT_FAILURE)
                exit(EXIT_FAILURE);
        }
        close(pipefd1[0]);
        //close(pipefd1[1]);
        //close(pipefd2[0]);
        close(pipefd2[1]);
        //sigprocmask(SIG_SETMASK, &mask, &prev);
        debug("%s", "program executed");
        /*if(error == -1){
            exitfork(EXIT_FAILURE, inputfd, outputfd);
        }*/

        if(inpos > 0 && inpos > outpos && inputfd < 0){
            inpos = trimwhitespace(inpos);
            inputfd = open(inpos, O_RDONLY);
            dup2(inputfd, STDIN_FILENO);
            if(inputfd < 0){
                //debug("%d", redirection);
                printf(SYNTAX_ERROR, "invalid file name");
                exitfork(EXIT_FAILURE, inputfd, outputfd);
            }
            else{
                char c;
                while((c = getchar()) != EOF)
                    putchar(c);
            }
        }
        /*else if(inpos > 0 && inputfd >= 0){
            printf(SYNTAX_ERROR, "redirection < cannot be used after a pipeline");
        }*/
        exitfork(EXIT_SUCCESS, inputfd, outputfd);
    }
    else{
        //close(pipefd1[0]);
        //close(pipefd1[1]);
        //close(pipefd2[0]);
        close(pipefd2[1]);
    }
    //setpgid(childpid, getpid());
    //tcsetpgrp(0, getpid());
    memcpy(pipefd1, pipefd2, sizeof(int) * 2);
    memset(pipefd2, -1, sizeof(int) * 2);
    debug("%s", token);
    if(token != NULL){
        token = trimwhitespace(token);
        findexecutable(token);
    }
    else{
        sigprocmask(SIG_BLOCK, &mask, &prev);
        while(childpid != pid){
            sigsuspend(&prev);
            if(WIFEXITED(status)){
                if(WEXITSTATUS(status) == EXIT_FAILURE)
                    exit(EXIT_FAILURE);
            }
        }
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    //for now just reap the child

    //signal(SIGINT, SIG_DFL);

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
