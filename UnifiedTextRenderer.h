#pragma once

#include "pico/stdlib.h"
#include "KanjiRenderer.h"
#include "GFXFontRenderer.h"
#include "ILI9341Driver.h"


uint8_t get_character_type(uint16_t character);

void RenderUnifiedText(uint16_t x, uint16_t y, uint16_t input[]);

void InitUnifiedText();