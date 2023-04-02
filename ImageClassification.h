#pragma once

#include "pico/stdlib.h"
#include "DataProcessing.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "model.h"

void InitTensorflow();

void GetNMostLikely(const int8_t input[64][64], uint16_t output[], uint16_t n);
