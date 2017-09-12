#include "hw1.h"
#include "debug.h"

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
int isvalidchar(char symbol, int ciphertype);

unsigned short validargs(int argc, char **argv) {
    short return_state = 0x0000;
    int count = 0;
    key = "\0";
    for(int i = 1; i < argc; i++)
    {
        count++;
        char* arg = *(argv+i);
        if (*(arg+2) != 0)
            return 0;
        if(*(arg+1) == 'h')
            return 0x8000;
        else if(*(arg+1) == 'p' && count == 1) //Polybius
            return_state = return_state | 0x0000;
        else if(*(arg+1) == 'f' && count == 1) //fractionated
            return_state = return_state | 0x4000;
        else if(*(arg+1) == 'e' && count == 2) //encrypt
            return_state = return_state | 0x0000;
        else if(*(arg+1) == 'd' && count == 2) //decrypt
            return_state = return_state | 0x2000;
        else if(*(arg+1) == 'k' && count > 2) //key
        {
            if(*key != 0)
                return 0;
            i++;
            arg = *(argv+i);
            while(*arg != '\n' && *arg != ' ' && *arg != '\0')
            {
                if(!isvalidchar(*arg, return_state & 0x4000))
                    return 0;
                arg++;
            }
            key = *(argv+i);
        }
        else if(*(arg+1) == 'r' && count > 2)
        {
            if(return_state & 0x4000)
                return 0;
            if(return_state & 0x00F0)
                return 0;
            i++;
            int j = 0;
            int num = 0;
            while(*(*(argv+i)+j) != '\0' && *(*(argv+i)+j) != ' ' && *(*(argv+i)+j) != '\n'){
                if(*(*(argv+i)+j) > '9' || *(*(argv+i)+j) < 0)
                    return 0;
                num *= 10;
                num += *(*(argv+i)+j) - '0';
                j++;
            }
            if(num > 15 || num < 9)
                return 0;
            return_state = return_state | num * 0x0010;
        }
        else if(*(arg+1) == 'c' && count > 2)
        {
            if(return_state & 0x4000)
                return 0;
            if(return_state & 0x000F)
                return 0;
            i++;
            int j = 0;
            int num = 0;
            while(*(*(argv+i)+j) != '\0' && *(*(argv+i)+j) != ' ' && *(*(argv+i)+j) != '\n'){
                if(*(*(argv+i)+j) > '9' || *(*(argv+i)+j) < 0)
                    return 0;
                num *= 10;
                num += *(*(argv+i)+j) - '0';
                j++;
            }
            if(num > 15 || num < 9)
                return 0;
            return_state = return_state | num * 0x0001;
        }
        else
        {
            count = 0;
        }
    }
    if (!(return_state & 0x00FF) && !(return_state & 0x4000))
        return_state = return_state | 0x00AA;
    if(count < 2)
        return 0;
    char *temptable;
    if(return_state & 0x4000) //fractionated
        temptable = fm_key;
    else //Polybius
    {
        int j = 0;
        while(*(polybius_alphabet+j) > 0)
            j++;
        if(((return_state & 0x00F0)/ 0x0010) * (return_state & 0x000F) < j)
            return 0;
        temptable = polybius_table;
    }
    int j = 0;
    while(*(temptable+j) > 0)
    {
        *(temptable+j) = 0;
        j++;
    }
    return return_state;
}

int isvalidchar(char symbol, int ciphertype)
{
    const char *tempalphabet;
    char *repeatcheck;
    if(ciphertype) //fractionated
    {
        tempalphabet = fm_alphabet;
        repeatcheck = fm_key;
    }

    else //Polybius
    {
        tempalphabet = polybius_alphabet;
        repeatcheck = polybius_table;
    }
    while(*repeatcheck != 0)
    {
        if(*repeatcheck == symbol) //check if repeat
            return 0;
        repeatcheck++;
    }
    *repeatcheck = symbol;

    int pos = 0;
    while(*(tempalphabet+pos) != '\0')
    {
        if(*(tempalphabet+pos) == symbol)
            return 1;
        pos++;
    }
    return 0;
}

int generatepolybiustable(short mode)
{
    int key_length = 0;
    while (*(key+key_length))
    {
        *(polybius_table+key_length) = *(key+key_length);
        key_length++;
    }
    int rows = (mode & 0x00F0) / 0x0010;
    int cols = mode & 0x000F;
    int table_offset = 0; //offset from key insertion *SHOULD BE NEGATIVE*
    int alphabet_end = 0;
    for(int i = key_length; i < rows * cols; i++)
    {
        if(!*(polybius_alphabet+i-key_length) || alphabet_end)
        {
            alphabet_end = 1;
            *(polybius_table+i+table_offset) = 0;
            continue;
        }
        *(polybius_table+i+table_offset) = *(polybius_alphabet+i-key_length);
        for(int j = 0; j < key_length; j++)
        {
            if(*(polybius_alphabet+i-key_length) == *(key+j))
            {
                table_offset--;
            }
        }
    }
    //printf("%s\n", polybius_table);
    return 1;
}

int encryptpolybius(short mode, char input)
{
    int cols = mode & 0x000F;
    if(input == ' ' || input == '\n' || input == '\t')
        printf("%c", input);
    else
    {
        int j = 0;
        while(*(polybius_table+j) != input)
        {
            if(!*(polybius_table))
                return 0;
            j++;
        }
        printf("%X%X", j/cols, j%cols);
    }
    return 1;
}

int decryptpolybius(short mode, int row, int col)
{
    //printf("%d%d\n", row, col);
    int cols = mode & 0x000F;
    int pos = row * cols + col;
    printf("%c", *(polybius_table+pos));
    return 1;
}

int generatemorsetable()
{
    int key_length = 0;
    for(int i = 0; *(key+i); i++)
    {
        *(fm_key+i) = *(key+i);
        key_length++;
    }
    int offset = 0; //NEGATIVE SO ADD!!!!
    for(int i = key_length; *(fm_alphabet+i-key_length); i++)
    {
        *(fm_key+i+offset) = *(fm_alphabet+i-key_length);
        for(int j = 0; j < key_length; j++)
        {
            if(*(fm_alphabet+i-key_length) == *(key+j))
                offset--;
        }
    }
    debug("%s", fm_key);
    return 1;
}

int encryptmorse(char *buffer, char input)
{
    if(input == ' ' || input == '\t' || input == '\n')
    {
        debug("%c", input);
        int insert_count = 0;
        int i = 0;
        while(insert_count < 1 && i < 8)
        {
            debug("%c", *(buffer+i));
            if(!*(buffer+i))
            {
                *(buffer+i) = 'x';
                insert_count++;
            }
            i++;
        }
        debug("buffer: %s", buffer);
    }
    else
    {
        //printf("%s\n", *(morse_table+(input-'!')));
        if(!*(morse_table+(input-'!')))
            return 0;
        //printf("success");
        const char *morse_str;

        morse_str = *(morse_table+(input-'!'));
        //printf("success");
        debug("%s\n", morse_str);

        int i = 0;
        int j = 0;
        while(*(morse_str+j) && i < 8)
        {
            //debug("%s", (buffer+i));
            if(!*(buffer+i))
            {
                *(buffer+i) = *(morse_str+j);
                j++;
            }
            i++;
        }
        //printf("loop success");
    }

    return 1;
}

int decryptmorse(char *buffer, char input, size_t length)
{
    /*if(input == '\n')
    {
        printf("\n");
        return 1;
    }*/
    //debug("current input: %c", input);
    int index;
    for(index = 0; *(fm_key+index) && *(fm_key+index) != input; index++);
    //debug("index: %d", index);
    const char *fm_str = *(fractionated_table+index);
    int i = 0;
    int j = 0;
    int count;
    while(*(fm_str+j) && i < length)
    {
        //debug("%s", (buffer+i));
        if(!*(buffer+i))
        {
            *(buffer+i) = *(fm_str+j);
            j++;
        }
        i++;
    }
    //debug("buffer state: %s", buffer);
    return 1;
}

int getindex(char * buffer, char value, size_t length)
{
    int i;
    for(i = 0; *(buffer+i) != value && i < length; i++);
    if(i == length)
        return -1;
    else
        return i;
}

int bufferencrypt(char *buffer)
{
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
    return 1;
}