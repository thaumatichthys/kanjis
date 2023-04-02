#include "ImageClassification.h"


namespace {
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *model_input = nullptr;
    TfLiteTensor *model_output = nullptr;

    constexpr int kTensorArenaSize = 170000;
    uint8_t *tensor_arena = nullptr;
}

void InitTensorflow() {
    stdio_init_all();
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;
    tensor_arena = (uint8_t*) malloc(kTensorArenaSize);
    model = tflite::GetModel(kanjis4_0_int8);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        error_reporter->Report("yer model version doesnt match bruv");
        while(1);
    }
    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model,
        resolver,
        tensor_arena,
        kTensorArenaSize,
        error_reporter
    );
    interpreter = &static_interpreter;
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        error_reporter->Report("allocate tensors failed bamnogus");
        while(1);
    }
    model_input = interpreter->input(0);
    model_output = interpreter->output(0);
}

// run inference
void GetNMostLikely(const int8_t input[64][64], uint16_t output[], uint16_t n) {
    // flatten and insert the data
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++) {
            long index = 64 * y + x;
            model_input->data.int8[index] = input[y][x];
        }
    }
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        error_reporter->Report("invoking failed bruv\n");
        while(1);
    }
    find_n_largest(model_output->data.int8, output, 2500, n);
}
