#include "ProgressBar.h"


uint8_t n_bars = 0;
bool initialized = false;
float progress[MAX_BARS];
float increment[MAX_BARS];
uint16_t duration[MAX_BARS];
bool running[MAX_BARS];
uint16_t xcoord[MAX_BARS];
uint16_t ycoord[MAX_BARS];

ILI9341 display;


ProgressBar::ProgressBar(uint16_t x, uint16_t y) {
    this->id = n_bars;
    n_bars++;
    xcoord[this->id] = x;
    ycoord[this->id] = y;
    if (!initialized)
        AddTask(ProgressBar::update, 50);
    initialized = true;
    progress[this->id] = 0.0f;
    running[this->id] = false;
}

void ProgressBar::draw(uint8_t id, float t, uint16_t colour) {
    uint16_t x = xcoord[id] + 20 * sin(t);
    uint16_t y = ycoord[id] + 20 * cos(t);
    display.FillSmallArea(x - 1, x + 1, y - 1, y + 1, colour);
}

void ProgressBar::update() {
    for (int i = 0; i < n_bars; i++) {
        if (running[i]) {
            float prog = progress[i];
            draw(i, prog, 0xFFFF);
            progress[i] += increment[i];
            if (prog > (2 * pi)) 
                running[i] = false;
        }
    }
    AddTask(ProgressBar::update, 10);
}

void ProgressBar::Start(uint16_t duration) {
    if (running[this->id])
        return;
    running[this->id] = true;
    progress[this->id] = 0.0f;
    increment[this->id] = (20 * pi) / duration;
}

void ProgressBar::Stop() {
    progress[this->id] = 0.0f;
    if (!running[this->id])
        return;
    running[this->id] = false;
    //for (float prog = 0.0f; prog < (2 * pi); prog += increment[this->id]) {
        //ProgressBar::draw(this->id, prog, 0);
    //}
}

void ProgressBar::Change(float state, uint16_t colour) {
    while (progress[this->id] < state) {
        progress[this->id] += 0.005;
        ProgressBar::draw(this->id, 2 * pi * progress[this->id], colour);
    }
}