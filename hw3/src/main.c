#include <stdio.h>
#include "sfmm.h"
#include "debug.h"

int main(int argc, char const *argv[]) {

    sf_mem_init();



    sf_malloc(sizeof(long));
    double* ptr = sf_malloc(sizeof(double)*10);
    //double* ptr1 = sf_malloc(sizeof(double));
    sf_malloc(sizeof(char));

    //sf_snapshot();

    //*ptr = 320320320e-320;

    printf("%f\n", *ptr);

    //sf_free(ptr1);
    //sf_snapshot();
    sf_free(ptr);
    //sf_snapshot();

    sf_mem_fini();

    return EXIT_SUCCESS;
}
