#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "bits.h"


// Get the nth bit
uint16_t getbit(uint16_t number, int n) {
    // TODO: Your code here
    return (number >> n) % 2;
}

// Get bits that are the given number of bits wide
uint16_t getbits(uint16_t number, int n, int wide) {
    // TODO: Your code here
    number = number >> n;
    int i = 2;
    for (; wide > 1; wide--){
        i *=2;
    }
    return number % i;
}

// Set the nth bit to the given bit value and return the result
uint16_t setbit(uint16_t number, int n) {
    return number | (1 << n);
}

// Clear the nth bit
uint16_t clearbit(uint16_t number, int n) {
    return number & ~(1 << n);
}

// Sign extend a number of the given bits to 16 bits
uint16_t sign_extend(uint16_t x, int bit_count) {
    uint16_t bit = getbit(x, bit_count - 1);
    if (bit == 1){
        for (int i = bit_count; i < 16; i++){
            x = setbit(x, i);
        }

    } else {
        for (int i = bit_count; i < 16; i++){
            clearbit(x, i);
        }
    }
    return x;
}

bool is_positive(uint16_t number) {
    return getbit(number, 15) == 0;
}

bool is_negative(uint16_t number) {
    return getbit(number, 15) == 1;
}
