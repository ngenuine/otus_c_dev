#include <stdio.h>
#include <inttypes.h>

/* int main(void) {
    int numbers[] = {1, 2, 3, 5};  // размер = 4
    int numbers0[] = { 0 };

    int numbers1[4] = {1, 2, 3, 5};
    numbers1[0] = 2;
    int sz1 = sizeof numbers1;
    int sz2 = sizeof numbers;
    int sz3 = sizeof numbers1;
} */

#include <stdio.h>
#include <stdint.h>

int main(void)
{
    int number = 5;
    int* ptr = &number;

    uintptr_t address = (uintptr_t)ptr;

    printf("Hex: %#jx\n", (uintmax_t)address);
    
    printf("Decimal:     %" PRIuPTR "\n", address);
    printf("Decimal:     %" PRIuPTR "\n", address);
    printf("Octal:       %#" PRIoPTR "\n", address);
    printf("Octal:       %" PRIoPTR "\n", address);
    printf("Hex lower:   %#" PRIxPTR "\n", address);
    printf("Hex lower:   %" PRIxPTR "\n", address);
    printf("Hex upper:   %#" PRIXPTR "\n", address);
    printf("Hex upper:   %" PRIXPTR "\n", address);
}
