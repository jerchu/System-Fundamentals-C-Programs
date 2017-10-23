#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>

#include "sfish.h"
#include "debug.h"

int main(int argc, char *argv[], char* envp[]) {
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
    char currentDir[256];
    char prevDir[256] = {0};
    getcwd(currentDir, 256 - strlen(" :: jerchu >>"));
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
                "pwd: prints the current working directory\n");
        }
        else if(strstr(input, "cd") != NULL) {
            char *tempinput = calloc(strlen(input), 1);
            tempinput = strcpy(tempinput, input);
            char *tokinput = strtok(input, " ");
            //if(strcmp(tokinput, "cd"))
                //printf(EXEC_NOT_FOUND, input);
            tokinput = strtok(NULL, " ");
            if(tokinput == NULL){
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
                memset(currentDir, 0, 256);
                getcwd(currentDir, 256);
                if(*tokinput){
                    //printf("%s%s\n", currentDir, tokinput);
                    //strcat(newdir, "./");
                    strcat(newdir, tokinput);
                    int success = chdir(newdir);
                    if(success > -1){
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
            printf(EXEC_NOT_FOUND, input);
        }

        // Readline mallocs the space for input. You must free it.
        rl_free(input);

    } while(!exited);

    debug("%s", "user entered 'exit'");

    return EXIT_SUCCESS;
}
