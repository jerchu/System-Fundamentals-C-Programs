#include "hw1.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the program
 * and will return a unsigned short (2 bytes) that will contain the
 * information necessary for the proper execution of the program.
 *
 * IF -p is given but no (-r) ROWS or (-c) COLUMNS are specified this function
 * MUST set the lower bits to the default value of 10. If one or the other
 * (rows/columns) is specified then you MUST keep that value rather than assigning the default.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return Refer to homework document for the return value of this function.
 */
unsigned short validargs(int argc, char **argv) {
    short return_state = 0x0000;
    for(int i = 0; i < argc; i++)
    {
        char* arg = *(argv+i);
        if(*(arg+1) == 'h')
            return 0x8000;
        else if(*(arg+1) == 'p' && i == 1)
        {
            return_state = return_state | 0x0000;
            return_state = return_state | 0x00AA;
        }
        else if(*(arg+1) == 'f' && i == 1)
            return_state = return_state | 0x4000;
        else if(*(arg+1) == 'e' && i == 2)
            return_state = return_state | 0x0000;
        else if(*(arg+1) == 'd' && i == 2)
            return_state = return_state | 0x2000;
        else if(*(arg+1) == 'k' && i > 2)
        {
            i++;
            arg = *(argv+i);
            int *alphabetlength;
            *alphabetlength = 0;
            while(*arg != '\n' && *arg != ' ' && *arg != '\0')
            {
                if(!isvalidchar(*arg, return_state & 0x4000, alphabetlength))
                    return 0;
                arg++;
            }
            extern key = *(argv+i);
        }
        else if(*arg+1)
    }
    return return_state;
}

int isvalidchar(char symbol, int ciphertype, int *alphabetlength)
{
    char *tempalphabet;
    char *repeatcheck;
    int alphalengthset = *alphabetlength > 0
    if(ciphertype) //fractionated
    {
        tempalphabet = extern fm_alphabet;
        repeatcheck = extern fm_key;
        if(*(repeatcheck+(symbol-'A'))) //check if repeat
            return 0;
        *(repeatcheck+(symbol-'A')) = 1; //is taken
    }

    else //Polybius
    {
        tempalphabet = extern polybius_alphabet;
        repeatcheck = extern polybius_table;
        if(*(repeatcheck+symbol)) //check if repeat
            return 0;
        *(repeatcheck+symbol) = 1; //is taken
    }

    while(*tempalphabet != '\0')
    {
        if(*tempalphabet == symbol)
            return 1;
        tempcipher++;
        if(!alphalengthset)
            *alphabetlength++;
    }
    return 0;
}
