#include "DataProcessing.h"


// Finds the n largest values in an array, and also sorts the outputs
void find_n_largest(int8_t input[], uint16_t output[], uint16_t input_size, uint16_t output_size) {
    bool* used_indices = (bool*) malloc(input_size * sizeof(bool));
    // ensure all elements are initialized as false
    for (uint16_t i = 0; i < input_size; i++)
        used_indices[i] = false;
    // iterate over all inputs n times, getting the maximum and
    // then removing it from the list each time
    for (uint16_t i = 0; i < output_size; i++) {
        int8_t largest = -128;
        uint16_t largest_index;
        for (uint16_t u = 0; u < input_size; u++) {
            if (!used_indices[u]) {
                if (input[u] >= largest) {
                    largest = input[u];
                    largest_index = u;
                }
            }
        }
        output[i] = largest_index;
        used_indices[largest_index] = true;
    }
    free(used_indices);
}

// Rounds unsigned integers to the nearest set number. Eg round_to_int(17, 10) returns 20
uint16_t round_to_int(uint16_t input, uint16_t multiple) {
    int remainder = input % multiple;
    int difference = multiple - remainder;
    if (remainder >= difference)
        return input + difference;
    else
        return input - remainder;
}
