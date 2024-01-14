#include "ProjectiveTransform.h"
#include "pico/stdlib.h"
#include "stdio.h"



void printM(float input[][SIZE + 1]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE + 1; j++) {
            printf("%7.2f ", input[i][j]);
        }
        printf("\n");
    }
}

void RowReduce(float input[][SIZE + 1]) {
    // this program does not check to make sure that every column has at least one non zero!!
    // turn the diagonal entries into ones, and turn the bottom triangle into zeros
    float divider;
    for (int column = 0; column < SIZE; column++) {
        for (int y = column; y < SIZE; y++) {
            divider = input[y][column];
            if (y == column) {
                // make sure divider isnt zero 
                if (divider * divider < 0.001) {
                    // if it is, find a row with a non zero in that spot, and add it on
                    for (int y1 = y; y1 < SIZE; y1++) {
                        if (input[y1][y] * input[y1][y] >= 0.001) {
                            // add that row to the zero-containing row to make it not zero
                            for (int j = 0; j < SIZE + 1; j++) {
                                input[y][j] += input[y1][j];
                            }
                            break;
                        }
                    }
                    divider = input[y][column];
                }
                for (int x = y; x < SIZE + 1; x++) {
                    input[y][x] /= divider;
                }
            }
            else {
                for (int x = 0; x < SIZE + 1; x++) {
                    input[y][x] -= divider * input[column][x];
                }
            }
        }
        printf("\n");
        printM(input);
        printf("\n");
    }
    // turn the upper triangle into zeros
    for (int column = SIZE - 1; column > 0; column--) {
        for (int y = column - 1; y >= 0; y--) {
            divider = input[y][column];
            for (int x = SIZE; x >= 0 + 1; x--) {
                input[y][x] -= divider * input[column][x];
            }
        }
    }
}

void GetProjectiveTransform(float output[3][3], float in1[8], float in2[8]) {
    // | a b c |   | x1 |       | x2 |
    // | d e f | * | y1 | = k * | y2 |
    // | g h 1 |   |  1 |       |  1 |
    float bigMatrix[8][9] = {
        // a          b          c     d          e          f      g                   h                   knowns
        {  in1[0],    in1[1],    1,    0,         0,         0,    -in2[0] * in1[0],   -in2[0] * in1[1],    in2[0] },
        {  0,         0,         0,    in1[0],    in1[1],    1,    -in2[1] * in1[0],   -in2[1] * in1[1],    in2[1] },
        {  in1[2],    in1[3],    1,    0,         0,         0,    -in2[2] * in1[2],   -in2[2] * in1[3],    in2[2] },
        {  0,         0,         0,    in1[2],    in1[3],    1,    -in2[3] * in1[2],   -in2[3] * in1[3],    in2[3] },
        {  in1[4],    in1[5],    1,    0,         0,         0,    -in2[4] * in1[4],   -in2[4] * in1[5],    in2[4] },
        {  0,         0,         0,    in1[4],    in1[5],    1,    -in2[5] * in1[4],   -in2[5] * in1[5],    in2[5] },
        {  in1[6],    in1[7],    1,    0,         0,         0,    -in2[6] * in1[6],   -in2[6] * in1[7],    in2[6] },
        {  0,         0,         0,    in1[6],    in1[7],    1,    -in2[7] * in1[6],   -in2[7] * in1[7],    in2[7] }
    };
    printM(bigMatrix);
    RowReduce(bigMatrix);
    output[0][0] = bigMatrix[0][8];
    output[1][0] = bigMatrix[1][8];
    output[2][0] = bigMatrix[2][8];
    output[0][1] = bigMatrix[3][8];
    output[1][1] = bigMatrix[4][8];
    output[2][1] = bigMatrix[5][8];
    output[0][2] = bigMatrix[6][8];
    output[1][2] = bigMatrix[7][8];
    output[2][2] = 1.0f;
    printf("after: \n");
    printM(bigMatrix);
}