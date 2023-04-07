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
uint64_t visual_drawing_area[192][3];
uint16_t guesses[6];
uint16_t currently_rendered[6];

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
    ready = false;
    GetNMostLikely(drawing_area, guesses, 6);
    ready = true;
    printf("inference took %dms\n", millis() - start);
    gpio_put(25, 0);
    a = 1 - a;
    
    printf("ranking: %d, %d, %d, %d, %d, %d\n", guesses[0], guesses[1], guesses[2], guesses[3], guesses[4], guesses[5]);
    printf("pix %d\n", drawing_area[0][0]);
}

void clear_drawing_area() {
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++) {
            drawing_area[x][y] = (int8_t) -128;
        }
    }
}

void clear() {
    clear_drawing_area();
    dp.FillSmallArea(0, 192, 48, 240, 0);
}

void clear_visual_buffer() {
    for (int i = 0; i < 192; i++) {
        for (int u = 0; u < 3; u++) {
            visual_drawing_area[i][u] = 0;            
        }
    }
}

void compute_results() {
    if (core1_active) {
        multicore_reset_core1();
        pb.Stop();
        core1_active = false;
    } 
}

void render_results() {
    kr.SetFontColour(0b1111100000000000);
    for (int i = 0; i < 6; i++) {
        if (i == 1) kr.SetFontColour(0);
        kr.Render(thumbnail_coords[i][0], thumbnail_coords[i][1], guesses[i]);
        currently_rendered[i] = guesses[i];
    }
}

void render_current() {
    kr.SetFontColour(0b1111100000000000);
    for (int i = 0; i < 6; i++) {
        if (i == 1) kr.SetFontColour(0);
        kr.Render(thumbnail_coords[i][0], thumbnail_coords[i][1], currently_rendered[i]);
    }
}

void write_visual_buffer(bool value, uint8_t x, uint8_t y) {
    if (value)
        visual_drawing_area[y][x / 64] |= ((uint64_t) 1U << (x % 64));
    else
        visual_drawing_area[y][x / 64] &= ~((uint64_t) 1U << (x % 64));
}

bool read_visual_buffer(uint8_t x, uint8_t y) {
    return (bool) (visual_drawing_area[y][x / 64] & ((uint64_t) 1U << (x % 64)));
}

bool screen = 0; // 0 == draw, 1 == info

void init_info_screen();
void update_info_screen();

void init_draw_screen() {
    dp.FillSmallArea(0, 320, 0, 240, 0xFFFF); // background
    dp.FillSmallArea(192, 320, 0, 48, 0b1000010000010000); // run button
    dp.FillSmallArea(0, 64, 0, 48, dp.RGBto16bit(127, 150, 127)); // clear button
    dp.FillSmallArea(65, 128, 0, 48, dp.RGBto16bit(200, 200, 200)); // erase button
    dp.FillSmallArea(129, 192, 0, 48, dp.RGBto16bit(127, 150, 127)); // cancel button
    clear();
    render_current();
    for (int x = 0; x < 192; x++) {
        for (int y = 0; y < 192; y++) {
            if (read_visual_buffer(x, y))
                dp.FillSmallArea(x - 1, x + 1, y + 48 - 1, y + 48 + 1, 0xFFFF);
        }
    }
    sleep_ms(300);
}

void update_draw_screen() {
    uint16_t x;
    uint16_t y;
    if (dp.ReadTouch(&x, &y)) {
        if (x < 192 && y > 48) {
            drawing_area[64 - (y - 48) / 3 - 1][x / 3] = 127;
            dp.FillSmallArea(x - 1, x + 1, y - 1, y + 1, 0xFFFF);

            write_visual_buffer(true, x, y - 48);

            ready = false;
            compute_results();
        }
        else if (x < 64 && y < 48) {
            clear();
            clear_visual_buffer();
            compute_results();
        }
        else if (x > 192 && y < 48) {
            render_results();
        }
        else if (x > 192 && y > 48) {
            screen = 1;
            init_info_screen();
        }
    }
    else if (!core1_active) {
        multicore_launch_core1(multicore_task);
        pb.Start(800);
        core1_active = true;
    }
}

void init_info_screen() {
    dp.FillSmallArea(0, 320, 0, 240, 0xFFFF);
    dp.FillSmallArea(256, 320, 208, 240, 0x7777);
    kr.Render(100, 100, 123);
    sleep_ms(300);
}

void update_info_screen() {
    uint16_t x;
    uint16_t y;
    if (dp.ReadTouch(&x, &y)) {
        if (x > 256 && y > 208) {
            screen = 0;
            init_draw_screen();
        }
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
    kr.SetFontColour(0);
    kr.OpenFile("64x64.bruh", 64);
    kr.SetDisplayHandler(buf_handler);
    clear_drawing_area();
    GetNMostLikely(drawing_area, guesses, 6);
    render_results();
    init_draw_screen();
    while(1) {
        if (!screen)
            update_draw_screen();
        else
            update_info_screen();
        UpdateTasks();
    }
}