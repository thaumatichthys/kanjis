#include "GFXFontRenderer.h"


void GFXFontRenderer::IndexToXY(uint32_t index, uint16_t *x, uint16_t *y) {

}

uint32_t GFXFontRenderer::XYToIndex(uint16_t x, uint16_t y) {
    return x * SCREEN_X_RES + y;
}

void GFXFontRenderer::SetFont(GFXfont font, uint16_t colour) {
    this->font_colour = colour;
    this->font = font;
}

uint16_t* temp_display_buffer;

void dummy_write_handler(uint16_t x, uint16_t y, uint16_t colour) {
    uint32_t index = GFXFontRenderer::XYToIndex(x, y);
    if (index < SCREEN_X_RES * SCREEN_Y_RES)
        temp_display_buffer[index] = colour;
}

void GFXFontRenderer::RenderChar(uint16_t *display_buffer, uint16_t x, uint16_t y, const char character) {
    void (*temp)(uint16_t x, uint16_t y, uint16_t colour) = this->write_pixel_func;
    this->write_pixel_func = dummy_write_handler;
    temp_display_buffer = display_buffer;
    this->RenderChar(x, y, character);
    this->write_pixel_func = temp;
}

void GFXFontRenderer::RenderText(uint16_t *display_buffer, uint16_t x, uint16_t y, const char* ascii) {
    void (*temp)(uint16_t x, uint16_t y, uint16_t colour) = this->write_pixel_func;
    this->write_pixel_func = dummy_write_handler;
    temp_display_buffer = display_buffer;
    this->RenderText(x, y, ascii);
    this->write_pixel_func = temp;
}

void GFXFontRenderer::SetDisplayHandler(void (*handler)(uint16_t x, uint16_t y, uint16_t colour)) {
    this->write_pixel_func = handler;
}

void GFXFontRenderer::RenderChar(uint16_t x, uint16_t y, const char character) {
    GFXglyph glyph = this->font.glyph[character];
    uint16_t bitmap_area_size = (glyph.width * glyph.height) / 8;
    for (int y_offset = 0; y_offset < glyph.height; y_offset++) {
        for (int x_offset = 0; x_offset < glyph.width; x_offset++) {
            uint16_t yval = y - (glyph.height + glyph.yOffset);
            uint16_t xval = x - (glyph.width + glyph.xOffset);
            uint16_t bit_offset = y_offset * glyph.width + x_offset;
            uint16_t byte_offset = bit_offset >> 3; // divide by 8
            if (this->font.bitmap[glyph.bitmapOffset + byte_offset] & (1U << (7 - (bit_offset % 8)))) {
                uint16_t pixel_x = x + x_offset;
                uint16_t pixel_y = yval + (glyph.height - y_offset - 1);
                if (0 <= pixel_x && pixel_x < SCREEN_Y_RES && 0 <= pixel_y && pixel_y < SCREEN_X_RES)
                    this->write_pixel_func(pixel_x, pixel_y, this->font_colour);
            }
        }
    }
}

void GFXFontRenderer::RenderText(uint16_t x, uint16_t y, const char* ascii) {
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;
    for (int i = 0; ascii[i] != 0; i++) {
        if (ascii[i] == '\n') {
            cursor_y -= this->font.yAdvance;
            cursor_x = x;
        }
        else {
            const char character = ascii[i] - this->font.first;
            GFXglyph glyph = this->font.glyph[character];
            this->RenderChar(cursor_x, cursor_y, character);
            cursor_x += glyph.xAdvance;
        }
    }
}