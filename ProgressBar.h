#pragma once

#include "pico/stdlib.h"
#include "TaskScheduler.h"
#include "ILI9341Driver.h"
#include "math.h"


#define MAX_BARS 10
#define pi 3.142f

class ProgressBar {
    static void update();
    static void draw(uint8_t id, float t, uint16_t colour);
    uint8_t id;
    public:
    ProgressBar(uint16_t x, uint16_t y);
    void Start(uint16_t duration);
    void Stop();
    void Change(float state, uint16_t colour);
};