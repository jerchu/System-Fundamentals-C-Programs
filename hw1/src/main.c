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
            long space = 0;
            char *buffer = polybius_table;
            char checkline;
            /*if(!decryptmorse(buffer, checkline))
                return EXIT_FAILURE;*/
            int prep_print_space = 0;
            int prev_nl = 0;
            while((checkline = getchar()) != EOF)
            {
                debug("prep_print_space: %d", prep_print_space);
                if(checkline == '\n'){
                    int count;
                    for(count = 0; *(buffer+count); count++);
                    if(count){
                        int valid = 0;
                        for(int i = 0; i < 'z' - '!'; i++)
                        {
                            int equal = 2;
                            for(int j = 0; *(buffer+j) && *(*(morse_table+i)+j); j++)
                            {
                                if(*(buffer+j) != *(*(morse_table+i)+j))
                                {
                                    equal = 0;
                                    break;
                                }
                                if(*(buffer+j+1) == '\0' && *(*(morse_table+i)+j+1) == '\0')
                                {
                                    equal = 1;
                                    break;
                                }
                            }
                            if(equal == 1)
                            {
                                printf("%c", i + '!');
                                valid = 1;
                                break;
                            }
                        }
                        for(int i = 0; *(buffer+i); i++)
                            *(buffer+i) = 0;
                        if(!valid)
                            return EXIT_FAILURE;
                    }
                    printf("\n");
                    prep_print_space = 0;
                }
                if(prep_print_space){
                    printf(" ");
                    prep_print_space = 0;
                }
                if(checkline != '\n' && !decryptmorse(buffer, checkline, sizeof(polybius_table)))
                    return EXIT_FAILURE;
                int index;
                while((index = getindex(buffer, 'x', sizeof(polybius_table)))+1 > 0)
                {
                    debug("buffer: %s", buffer);
                    if(index == 0)
                    {
                        prep_print_space = 1;
                        *(buffer+index) = 0;
                    }
                    else
                    {
                        if(prep_print_space)
                        {
                            printf(" ");
                            prep_print_space = 0;
                        }
                        *(buffer+index) = 0;
                        debug("buffer: %s", buffer);
                        int valid = 0;
                        for(int i = 0; i < 'z' - '!'; i++)
                        {
                            int equal = 2;
                            for(int j = 0; *(buffer+j) && *(*(morse_table+i)+j); j++)
                            {
                                if(*(buffer+j) != *(*(morse_table+i)+j))
                                {
                                    equal = 0;
                                    break;
                                }
                                if(*(buffer+j+1) == '\0' && *(*(morse_table+i)+j+1) == '\0')
                                {
                                    equal = 1;
                                    break;
                                }
                            }
                            if(equal == 1)
                            {
                                printf("%c", i + '!');
                                valid = 1;
                                break;
                            }
                        }
                        if(!valid)
                            return EXIT_FAILURE;
                    }
                    int count;
                    for (count = index+1; *(buffer+count); count++);
                    for(int j = 0; j < count; j++)
                    {
                        if(j+index+1 < count)
                            *(buffer+j) = *(buffer+index+1+j);
                        else
                            *(buffer+j) = 0;
                    }
                }
            }
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
                    //debug("I chose last_char_space because %d", last_char_space);
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
                    bufferencrypt(buffer);
                    if(!encryptmorse(buffer, checkline))
                        return EXIT_FAILURE;
                }
                if(checkline == ' ' || checkline == '\t')
                    last_char_space = 1;
                else
                {
                    last_char_space = 0;
                }
                bufferencrypt(buffer);
                if(checkline == '\n')
                {
                    printf("\n");
                    for(int i = 0; *(buffer+i); i++){
                        *(buffer+i) = 0;
                    }
                    last_char_space = 1;
                }
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