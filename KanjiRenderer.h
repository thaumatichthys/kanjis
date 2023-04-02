#pragma once

#include "pico/stdlib.h"
#include "cstdlib"
#include "FileReader.h"
#include "config.h"
#include "ILI9341Driver.h"


class KanjiRenderer {
    void (*write_image_func)(uint16_t* buffer, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    FileReader filereader;
    uint8_t side_length;
    uint16_t colour;
    uint16_t* buffer;
    public:
    void SetFontColour(uint16_t colour);
    void SetDisplayHandler(void (*handler)(uint16_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h));
    void OpenFile(const char* filename, uint8_t kanji_size);
    void Render(uint16_t x, uint16_t y, uint16_t index);
};