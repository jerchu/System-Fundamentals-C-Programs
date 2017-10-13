#include <stdio.h>
#include "sfmm.h"
#include "debug.h"

int main(int argc, char const *argv[]) {

    sf_mem_init();



    //sf_malloc(sizeof(long));
    //double* ptr = sf_malloc(sizeof(double));
    double* next_page = sf_malloc(PAGE_SZ * 4);
    //double* ptr1 = sf_malloc(sizeof(double));
    //sf_malloc(sizeof(char));

    sf_snapshot();

    //*ptr = 1.337;//320320320e-320;

    //printf("%1.3f\n", *ptr);

    //sf_blockprint((void *)y - 8);

    //sf_free(ptr1);
    //sf_snapshot();
    sf_free(next_page);
    //sf_realloc(ptr, 0);
    sf_snapshot();

    sf_mem_fini();

    return EXIT_SUCCESS;
}
