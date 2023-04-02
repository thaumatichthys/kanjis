#pragma once


#include "pico/stdlib.h"
#include "cstdlib" // idk why pico/stdlib.h doesn't cover this, but malloc and free won't compile without this


void find_n_largest(int8_t input[], uint16_t output[], uint16_t input_size, uint16_t output_size);
