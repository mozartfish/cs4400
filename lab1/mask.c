/*
Hello world in C
*/
#include <stdio.h>
int main() {
    unsigned int bits = 0xAABBCCDD;
    unsigned char MSB = bits >> 24;
    unsigned char shifted = bits >> 20;
    shifted = shifted & 0x03F;

    /*Extract the two most significant bytes of the variable called bits into an unsigned short.*/
    unsigned short sbytes = bits >> 16;
    /*Extract the two least significant bits of the variable called bits into an unsigned char*/
    unsigned char cbytes = bits & 0x3;
    /*Extract bits 7-9 (inclusive, 0-based) into an unsigned char.*/
    unsigned char ebits = (bits >> 7) & 0x7;
    printf("%x\n", shifted);
    return 0;
}