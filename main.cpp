#include "stdio.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"



#include "test_images.h"
#include "images.hpp"

#include "ILI9341Driver.h"
#include "GFXFontRenderer.h"
#include "ImageClassification.h"

#include "config.h"
#include "FreeSansOblique24pt7b.h"


#include "FileReader.h"
#include "KanjiRenderer.h"


ILI9341 dp;
GFXFontRenderer tf;
KanjiRenderer kr;

int8_t drawing_area[64][64];
uint16_t guesses[6];

bool changed = false;
bool dchanged = false;

#define millis() to_ms_since_boot(get_absolute_time())

void display_handler(uint16_t x, uint16_t y, uint16_t colour) {
    dp.WritePixel(x, y, colour);
}

void buf_handler(uint16_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    dp.WriteImage(img, x, y, w, h);
}

int a = 1;
void multicore_task() {
    while(1) {
        GetNMostLikely(drawing_area, guesses, 6);
        changed = true;
        a = 1 - a;
        gpio_put(25, a);
        printf("ranking: %d, %d, %d, %d, %d, %d\n", guesses[0], guesses[1], guesses[2], guesses[3], guesses[4], guesses[5]);
        printf("pix %d\n", drawing_area[0][0]);
    }
    
}

void clear() {
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++) {
            drawing_area[x][y] = (int8_t) -128;
        }
    }
    dp.FillArea(0, 64 * 3, 0, 64 * 3, 0);
    dchanged = true;
}

int main() {
    vreg_set_voltage(VREG_VOLTAGE_1_25);
    set_sys_clock_khz(360000, true);        // overclock to 360 MHz

    stdio_init_all();

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);

    InitTensorflow();

    FileReader::Mount();


    dp.Init();
    
    tf.SetFont(FreeSansOblique24pt7b, 0x0);
    tf.SetDisplayHandler(display_handler);

    dp.FillArea(0, 320, 0, 240, 0xFFFF);

    tf.RenderText(100, 100, "hey!\n()-+={}|");


    kr.SetFontColour(0);
    kr.OpenFile("64x64.bruh", 64);
    kr.SetDisplayHandler(buf_handler);
    kr.Render(192, 192, 1234);

    //tf.RenderChar(display_buffer, 50, 50, 'a');

    //dp.WriteImage(display_buffer);
    //sleep_ms(3000);

    clear();

    multicore_launch_core1(multicore_task);

    uint32_t last_time = millis();
    while(1) {
        //gpio_put(25, 1);
        //sleep_ms(100);
        //gpio_put(25, 0);

        uint16_t x;
        uint16_t y;
        if (dp.ReadTouch(&x, &y)) {
            if (x < 192 && y < 192) {
                drawing_area[64 - y / 3 - 1][x / 3] = 127;
                dp.FillArea(x - 1, x + 1, y - 1, y + 1, 0xFFFF);
            }
            else if (x < 192 && y > 192) {
                clear();
            }
            dchanged = true;
            last_time = millis();
        }
        
        else if (dchanged && (millis() - last_time > 2000)) {
            //dp.FillArea(192, 0, 320, 192, 0xFFFF);
            kr.SetFontColour(0b1111100000000000);
            kr.Render(192, 0, guesses[0]);
            sleep_ms(200);
            kr.SetFontColour(0);
            kr.Render(192 + 64, 0, guesses[1]);
            kr.Render(192, 64, guesses[2]);
            kr.Render(192 + 64, 64, guesses[3]);
            kr.Render(192, 128, guesses[4]);
            kr.Render(192 + 64, 128, guesses[5]);
            last_time = millis();
            dchanged = false;
        }
    }
}