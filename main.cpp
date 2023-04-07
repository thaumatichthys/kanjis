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
#include "TaskScheduler.h"
#include "ProgressBar.h"


#include "config.h"
#include "FreeSansOblique24pt7b.h"


#include "FileReader.h"
#include "KanjiRenderer.h"


ILI9341 dp;
GFXFontRenderer tf;
KanjiRenderer kr;

int8_t drawing_area[64][64];
uint16_t guesses[6];

#define millis() to_ms_since_boot(get_absolute_time())

void display_handler(uint16_t x, uint16_t y, uint16_t colour) {
    dp.WritePixel(x, y, colour);
}

void buf_handler(uint16_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    dp.WriteSmallImage(img, x, y, w, h);
}

// todo: fix the stupid crashes


const uint16_t thumbnail_coords[][2] {
    { 257, 176 },
    { 257, 112 },
    { 257, 48 },
    { 193, 176 },
    { 193, 112 },
    { 193, 48 }
};

int a = 1;
bool core1_active = false;
bool ready = false;

ProgressBar pb(256, 24);

void multicore_task() {
    gpio_put(25, 1);
    long start = millis();
    GetNMostLikely(drawing_area, guesses, 6);
    ready = true;
    printf("inference took %dms\n", millis() - start);
    gpio_put(25, 0);
    a = 1 - a;
    
    printf("ranking: %d, %d, %d, %d, %d, %d\n", guesses[0], guesses[1], guesses[2], guesses[3], guesses[4], guesses[5]);
    printf("pix %d\n", drawing_area[0][0]);
}

void clear() {
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++) {
            drawing_area[x][y] = (int8_t) -128;
        }
    }
    dp.FillSmallArea(0, 192, 48, 240, 0);
}

void compute_results() {
    if (core1_active) {
        multicore_reset_core1();
        pb.Stop();
        core1_active = false;
    } 
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
    dp.FillSmallArea(0, 320, 0, 240, 0xFFFF); // background
    dp.FillSmallArea(192, 320, 0, 48, 0b1000010000010000); // run button
    dp.FillSmallArea(0, 64, 0, 48, dp.RGBto16bit(127, 150, 127)); // clear button
    dp.FillSmallArea(65, 128, 0, 48, dp.RGBto16bit(200, 200, 200)); // erase button
    dp.FillSmallArea(129, 192, 0, 48, dp.RGBto16bit(127, 150, 127)); // cancel button
    kr.SetFontColour(0);
    kr.OpenFile("64x64.bruh", 64);
    kr.SetDisplayHandler(buf_handler);
    clear();
    while(1) {
        uint16_t x;
        uint16_t y;
        if (dp.ReadTouch(&x, &y)) {
            if (x < 192 && y > 48) {
                drawing_area[64 - (y - 48) / 3 - 1][x / 3] = 127;
                dp.FillSmallArea(x - 1, x + 1, y - 1, y + 1, 0xFFFF);
                ready = false;
                compute_results();
            }
            else if (x < 64 && y < 48) {
                clear();
                compute_results();
            }
            else if (x > 192 && y < 48) {
                kr.SetFontColour(0b1111100000000000);
                for (int i = 0; i < 6; i++) {
                    if (i == 1) kr.SetFontColour(0);
                    kr.Render(thumbnail_coords[i][0], thumbnail_coords[i][1], guesses[i]);
                }
            }
        }
        else if (!core1_active) {
            multicore_launch_core1(multicore_task);
            pb.Start(800);
            core1_active = true;
        }

        UpdateTasks();
    }
}