#include <stdlib.h>

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
        return EXIT_FAILURE;
    }

    if(mode & 0x4000) {
        printf("fractionated\n");
    }
    else{
        printf("Polybius\n");
    }

    if(mode & 0x2000) {
        printf("decrypt\n");
    }
    else{
        printf("encrypt\n");
    }

    if(mode & 0x00F0)
    {
        printf("rows: %d\n", (mode & 0x00F0) / 0x0010);
    }

    if(mode &0x000F)
    {
        printf("cols: %d\n", (mode & 0x000F));
    }

    printf("%s\n", key);

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */