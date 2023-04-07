#include "UnifiedTextRenderer.h"


GFXFontRenderer english;
KanjiRenderer kanji;
KanjiRenderer hiragana;
KanjiRenderer katakana;
ILI9341* display;


uint8_t get_character_type(uint16_t character) {
    if (character < 256)
        return 0; // english / ASCII
    else if (character >= 300 && character < 2800)
        return 1; // kanji
    else if (character >= 2800 && character < 2846)
        return 2; // hiragana
    else if (character )
}

void InitUnifiedText(ILI9341* dp) {
    display = dp;
    kanji.OpenFile("20x20.bruh", 20);

}

void RenderUnifiedText(uint16_t x, uint16_t y, uint16_t input[]) {
    uint16_t index = 0;
    while(input[index]) { // null terminated input
        // check for continuous string of a single type
        while()
    }
}
