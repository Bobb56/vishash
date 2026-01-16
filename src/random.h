#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>
#include <stdio.h>

#include "vishash.h"

uint32_t next_random_number(void);
void init_random_number(uint8_t* file_data, long int size);

#endif