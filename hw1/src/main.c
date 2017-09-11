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
        generatemorsetable();
        if(mode & 0x2000){ //decrypt
            printf("decrypt");
        }
        else{ //encrypt
            long space = 0;
            char *buffer = (char *)&space;
            char checkline = getchar();
            int last_char_space = 0;
            if(!encryptmorse(buffer, checkline))
                return EXIT_FAILURE;
            while((checkline = getchar()) != EOF)
            {
                if(last_char_space)
                {
                    debug("I chose last_char_space because %d", last_char_space);
                    if(!(checkline == ' ' || checkline == '\t'))
                    {
                        if(!encryptmorse(buffer, checkline))
                            return EXIT_FAILURE;
                    }
                }
                else
                {
                    int insert_count = 0;
                    int i = 0;
                    while(insert_count < 1 && i < 8)
                    {
                        debug("'%c'", *(buffer+i));
                        if(!*(buffer+i))
                        {
                            *(buffer+i) = 'x';
                            insert_count++;
                        }
                        i++;
                    }
                    if(!encryptmorse(buffer, checkline))
                        return EXIT_FAILURE;
                }
                if(checkline == ' ' || checkline == '\t')
                    last_char_space = 1;
                else
                {
                    last_char_space = 0;
                }
                while(*(buffer+2))
                {
                    debug("%c", *(buffer+2));
                    debug("%s", buffer);
                    int i = 0;
                    while(*(fm_key+i)){
                        int equal = 1;
                        for(int j = 0; j < 3; j++){
                            if(*(buffer+j) != *(*(fractionated_table+i)+j))
                                equal = 0;
                        }
                        if(equal){
                            debug("%c", *(fm_key+i));
                            printf("%c", *(fm_key+i));
                            break;
                        }
                        i++;

                    }
                    for(int j = 0; j < 8; j++)
                    {
                        if(j+3 < 8)
                            *(buffer+j) = *(buffer+3+j);
                        else
                            *(buffer+j) = 0;
                    }
                }
                if(checkline == '\n')
                    printf("\n");
            }
        }
    }
    else{ //Polybius
        generatepolybiustable(mode);
        if(mode & 0x2000) { //decrypt
            char checkline = getchar();
            int row = -1;
            int col = -1;
            while(checkline != EOF)
            {
                if(checkline == '\n' || checkline == ' ' || checkline == '\t')
                    printf("%c", checkline);
                else if(row < 0 || col < 0)
                {
                    if(row < 0)
                    {
                        if(checkline > 'A')
                            row = checkline - 'A' + 10;
                        else
                            row = checkline - '0';
                    }
                    else
                    {
                        if(checkline > 'A')
                            col = checkline - 'A' + 10;
                        else
                            col = checkline - '0';
                    }
                }
                if(row >= 0 && col >= 0)
                {
                    decryptpolybius(mode, row, col);
                    row = -1;
                    col = -1;
                }
                checkline = getchar();
            }
        }
        else{ //encrypt
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