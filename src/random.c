#include "random.h"

#define A 16807
#define C 42
#define M (((uint32_t)1 << 31) - 1)

static uint32_t random_number = 0;

uint32_t next_random_number(void) {
    random_number = (random_number * A + C) % M;
    return random_number;
}

void init_random_number(uint8_t* file_data, long int size) {
    random_number = 3;
    for (long int i = 0 ; i < size ; i++) {
        random_number = (random_number * A + file_data[i]) % M;
    }
    debug("Nombre aléatoire initial : %d\n", random_number);
}