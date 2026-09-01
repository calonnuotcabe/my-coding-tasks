#include <stdio.h>
#include <stdint.h>

int main(void) {
    uint8_t data = 0x42;

    printf("hex = 0x%02x\n", (unsigned)data);
    printf("char = %c\n", (char)data);

    return 0;
}