#include <stdio.h>
#include <stdlib.h>

int main() {
    int* a = (int*)malloc(10 * sizeof(int));
    int* b = (int*)malloc(10 * sizeof(int));
    printf("a = %p, b = %p\n", a, b);
    free(a);
    a = (int*)malloc(20 * sizeof(int));
    printf("a = %p, b = %p\n", a, b);
    return 0;
}