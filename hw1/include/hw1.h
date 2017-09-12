#ifndef HW_H
#define HW_H

#include "const.h"

#endif

int generatepolybiustable(short mode);

int encryptpolybius(short mode, char input);

int decryptpolybius(short mode, int row, int col);

int generatemorsetable();

int encryptmorse(char *buffer, char input);

int decryptmorse(char *buffer, char input, size_t length);

int getindex(char *buffer, char value, size_t length);

int bufferencrypt(char *buffer);