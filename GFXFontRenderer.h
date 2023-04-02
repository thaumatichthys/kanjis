#include "stdio.h"
#include "pico/stdlib.h"

#include "config.h"


// from adafruit GFX library github
typedef struct { // per glyph
    uint32_t bitmapOffset; ///< Pointer into GFXfont->bitmap
    uint8_t width;         ///< Bitmap dimensions in pixels
    uint8_t height;        ///< Bitmap dimensions in pixels
    uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
    int8_t xOffset;        ///< X dist from cursor pos to UL corner
    int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

typedef struct { // for the whole font
    uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
    GFXglyph *glyph;  ///< Glyph array
    uint16_t first;   ///< ASCII extents (first char)
    uint16_t last;    ///< ASCII extents (last char)
    uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

class GFXFontRenderer {
    //uint8_t cursor_x;
    //uint8_t cursor_y;
    void (*write_pixel_func)(uint16_t x, uint16_t y, uint16_t colour);
    public:
    GFXfont font;
    uint16_t font_colour;
    void SetFont(GFXfont font, uint16_t colour);
    static void IndexToXY(uint32_t index, uint16_t *x, uint16_t *y);
    static uint32_t XYToIndex(uint16_t x, uint16_t y);
    void RenderChar(uint16_t *display_buffer, uint16_t x, uint16_t y, const char character);
    void RenderText(uint16_t *display_buffer, uint16_t x, uint16_t y, const char* ascii);
    void SetDisplayHandler(void (*handler)(uint16_t x, uint16_t y, uint16_t colour));
    void RenderChar(uint16_t x, uint16_t y, const char character);
    void RenderText(uint16_t x, uint16_t y, const char* ascii);
};