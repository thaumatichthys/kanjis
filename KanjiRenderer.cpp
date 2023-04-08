#include "KanjiRenderer.h"


void KanjiRenderer::SetFontColour(uint16_t colour) {
    this->colour = colour;
}

// kanji_size: the side length of the kanji in the font (only squares for now)
void KanjiRenderer::OpenFile(const char* filename, uint16_t w, uint16_t h) {
    KanjiRenderer::filereader.OpenFile(filename);
    this->width = w;
    this->height = h;
}

void KanjiRenderer::Render(uint16_t x, uint16_t y, uint16_t index) {
    if (index >= 2500)
        return;
    int bits_per_glyph = this->width * this->height;
    int bytes_per_glyph = bits_per_glyph / 8 + ((bits_per_glyph % 8) ? 1 : 0); // basically it rounds up to the nearest byte
    int file_index = bytes_per_glyph * index;

    uint8_t* file_buffer = (uint8_t*) malloc(bytes_per_glyph * sizeof(uint8_t)); // sizeof is redundant here but idc
    uint bytes_read;

    this->filereader.ReadAtOffset(file_index, file_buffer, bytes_per_glyph * sizeof(uint8_t), &bytes_read);
    //void ILI9341::RenderBinary(const uint8_t* img, uint16_t colour, uint16_t background, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
    display_driver.RenderBinary(file_buffer, this->colour, 0xFFFF, x, y, this->width, this->height);
    free(file_buffer);
}