#pragma once 


#define SIZE 8


void RowReduce(float input[][SIZE + 1]);
void GetProjectiveTransform(float output[3][3], float in1[8], float in2[8]);