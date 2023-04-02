#include "KanjiRenderer.h"


void KanjiRenderer::SetFontColour(uint16_t colour) {
    this->colour = colour;
}

void KanjiRenderer::SetDisplayHandler(void (*handler)(uint16_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h)) {
    this->write_image_func = handler;
}

// kanji_size: the side length of the kanji in the font (only squares for now)
void KanjiRenderer::OpenFile(const char* filename, uint8_t kanji_size) {
    KanjiRenderer::filereader.OpenFile(filename);
    this->side_length = kanji_size;
    this->buffer = (uint16_t*) malloc(kanji_size * kanji_size * sizeof(uint16_t));
}

void KanjiRenderer::Render(uint16_t x, uint16_t y, uint16_t index) {
    if (index >= 2500)
        return;
    int bits_per_glyph = this->side_length * this->side_length;
    int bytes_per_glyph = bits_per_glyph / 8 + ((bits_per_glyph % 8) ? 1 : 0); // basically it rounds up to the nearest byte
    int file_index = bytes_per_glyph * index;
    const int width = this->side_length;

    uint8_t* file_buffer = (uint8_t*) malloc(bytes_per_glyph * sizeof(uint8_t)); // sizeof is redundant here but idc
    uint bytes_read;

    this->filereader.ReadAtOffset(file_index, file_buffer, bytes_per_glyph, &bytes_read);
    while(!ILI9341::dma_write_complete);
    for (int x_offset = 0; x_offset < this->side_length; x_offset++) {
        for (int y_offset = 0; y_offset < this->side_length; y_offset++) {
            int xpix = x + x_offset;
            int ypix = y - y_offset - 1;
            /*
            
            if ((file_buffer[current_file_index] & (1U << bit_offset)) && 0 <= xpix && xpix < SCREEN_Y_RES && 0 <= ypix && ypix < SCREEN_X_RES) {
                this->write_pixel_func(xpix, ypix, this->colour);
            }*/
            int bit_number = x_offset + width * y_offset;
            int bit_offset = bit_number % 8; // (the bit inside the byte)
            int current_file_index = bit_number / 8;
            if (0 <= xpix && xpix < SCREEN_Y_RES && 0 <= ypix && ypix < SCREEN_X_RES) {
                int buffer_index = xpix + this->side_length * ypix;
                if (file_buffer[current_file_index] & (1U << (8 - bit_offset - 1)))
                    this->buffer[buffer_index] = this->colour;
                else
                    this->buffer[buffer_index] = 0xFFFF;
            }
        }
    }
    this->write_image_func(this->buffer, x, y, this->side_length, this->side_length);
    free(file_buffer);
}