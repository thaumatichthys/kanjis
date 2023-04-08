#pragma once

#include "pico/stdlib.h"
#include "cstdlib"
#include "FileReader.h"
#include "config.h"
#include "ILI9341Driver.h"


class KanjiRenderer {
    FileReader filereader;
    uint16_t height;
    uint16_t width;
    uint16_t colour;
    ILI9341 display_driver;
    public:
    void SetFontColour(uint16_t colour);
    void OpenFile(const char* filename, uint16_t w, uint16_t h);
    void Render(uint16_t x, uint16_t y, uint16_t index);
};