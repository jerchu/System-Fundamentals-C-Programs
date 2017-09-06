#include <stdlib.h>
#include <stdio.h>

#include "hw1.h"
#include "debug.h"
#include "const.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    unsigned short mode;

    mode = validargs(argc, argv);

    debug("Mode: 0x%X", mode);

    if(mode & 0x8000) {
        USAGE(*argv, EXIT_SUCCESS);
    }

    if(!mode){
        printf("an error occured\n");
        USAGE(*argv, EXIT_FAILURE);
    }

    if(mode & 0x4000) { //fractionated
        printf("fractionated\n");
    }
    else{ //Polybius
        printf("Polybius\n");
        generatepolybiustable(mode);
    }

    if(mode & 0x2000) { //decrypt
        printf("decrypt\n");
    }
    else{ //encrypt
        printf("encrypt\n");
        //printf("input: ");
        char checkline = getchar();
        //printf("output: ");
        if(!encryptpolybius(mode, checkline))
                return 0;
        while((checkline = getchar()) != EOF)
        {
            if(!encryptpolybius(mode, checkline))
                return 0;
        }
    }

    /*if(mode & 0x00F0)
    {
        printf("rows: %d\n", (mode & 0x00F0) / 0x0010);
    }

    if(mode &0x000F)
    {
        printf("cols: %d\n", (mode & 0x000F));
    }*/

    //printf("%s\n", key);

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */