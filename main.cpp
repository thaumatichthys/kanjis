#include "stdio.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"

#include "images.h"

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
KanjiRenderer kr; // tiles for selection
KanjiRenderer is; // info screen

int8_t drawing_area[64][64];
uint64_t visual_drawing_area[192][3];
uint16_t guesses[6];
uint16_t currently_rendered[6];
uint16_t selected_kanji;

int a = 1;
bool core1_active = false;
bool ready = false;
uint64_t last_updated_ms = 2000;

void display_handler(uint16_t x, uint16_t y, uint16_t colour) {
    dp.WritePixel(x, y, colour);
}

// todo: fix the stupid crashes


const uint16_t thumbnail_coords[][2] { // bottom left corners of tiles
    { 257, 176 },
    { 257, 112 },
    { 257, 48 },
    { 193, 176 },
    { 193, 112 },
    { 193, 48 }
};

ProgressBar pb(256, 24);

void multicore_task() {
    gpio_put(25, 1);
    long start = to_ms_since_boot(get_absolute_time());
    ready = false;
    GetNMostLikely(drawing_area, guesses, 6);
    ready = true;
    printf("inference took %dms\n", to_ms_since_boot(get_absolute_time()) - start);
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

void visual_clear() {
    dp.FillSmallArea(0, 192, 48, 240, 0);
}

void clear() {
    clear_drawing_area();
    visual_clear();
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

uint8_t screen = 0; // 0 == draw, 1 == info, 2 == status

void init_info_screen();
void update_info_screen();
void init_status_screen();
void update_status_screen();

void init_draw_screen() {
    dp.FillSmallArea(0, 320, 0, 240, 0xFFFF); // background
    dp.FillSmallArea(192, 320, 0, 48, 0b1000010000010000); // run button
    dp.FillSmallArea(0, 64, 0, 48, dp.RGBto16bit(127, 150, 127)); // clear button
    dp.FillSmallArea(65, 128, 0, 48, dp.RGBto16bit(200, 200, 200)); // erase button
    dp.FillSmallArea(129, 192, 0, 48, dp.RGBto16bit(127, 150, 127)); // cancel button
    visual_clear();
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
        else if (x >= 64 && x < 128 && y < 48) {
            // UNDO BUTTON
        }
        else if (x >= 128 && x < 192 && y < 48) {
            screen = 2;
            init_status_screen();
        }
        else if (x > 192 && y < 48) {
            render_results();
        }
        else if (x > 192 && y > 48) {
            screen = 1;
            for (int i = 0; i < 6; i++) {
                if ((thumbnail_coords[i][0] <= x) && (x < thumbnail_coords[i][0] + 64) && (thumbnail_coords[i][1] <= y) && (y < thumbnail_coords[i][1] + 64)) {
                    selected_kanji = currently_rendered[i];
                    break;
                }
            }
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
    is.Render(0, 0, selected_kanji);
    dp.FillSmallArea(256, 320, 208, 240, 0x7777);
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

void update_status_voltage() {
    if (screen != 2) return;
    char buf[] = "Batt: X.XXV";
    const uint8_t y_coord = 200;
    float battery_voltage = 2 * 3.3f * (float) adc_read() / 4096;
    snprintf(&buf[6], 5, "%f", battery_voltage);
    buf[10] = 'V';
    buf[11] = 0;
    dp.FillSmallArea(0, 240, y_coord - 20, y_coord + 40, 0xFFFF);
    tf.RenderText(0, y_coord, buf);
    AddTask(update_status_voltage, 1000);
}

void init_status_screen() {
    dp.FillSmallArea(0, 320, 0, 240, 0xFFFF);
    dp.FillSmallArea(256, 320, 208, 240, 0x7777);
    sleep_ms(300);
    update_status_voltage();
}

void update_status_screen() {
    uint16_t x;
    uint16_t y;
    if (dp.ReadTouch(&x, &y)) {
        if (x > 256 && y > 208) {
            screen = 0;
            init_draw_screen();
        }
    }
}

void cli_reboot() {
    if (getchar_timeout_us(0) == 'r') {
        reset_usb_boot(0, 0);
    }
    AddTask(cli_reboot, 500);
}

void disable_mosfet() {
    gpio_set_dir(PSU_ENABLE_PIN, GPIO_IN);
}

void power_off() {
    if ((!gpio_get(BUTTON_SENSE_PIN)) && to_ms_since_boot(get_absolute_time()) > 2000) {
        disable_mosfet();
    }

    AddTask(power_off, 30);
}

void update_watchdog() {
    watchdog_update();
    AddTask(update_watchdog, 500);
}

void sleep_task() {
    uint16_t dummy;
    uint64_t current_time = to_ms_since_boot(get_absolute_time());
    if (dp.ReadTouch(&dummy, &dummy))
        last_updated_ms = current_time;
    else if (current_time - last_updated_ms > SLEEP_TIMEOUT_MS) 
        disable_mosfet();
    AddTask(sleep_task, 100);
}

int main() {
    gpio_init(PSU_ENABLE_PIN);
    gpio_set_dir(PSU_ENABLE_PIN, GPIO_OUT);
    gpio_put(PSU_ENABLE_PIN, 0);

    vreg_set_voltage(VREG_VOLTAGE_1_25);
    set_sys_clock_khz(360000, true);        // overclock to 360 MHz

    stdio_init_all();
    watchdog_enable(4000, true);

    gpio_init(TFT_BACKLIGHT_PIN);
    gpio_set_dir(TFT_BACKLIGHT_PIN, GPIO_OUT);
    

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);
    gpio_init(BUTTON_SENSE_PIN);
    gpio_set_dir(BUTTON_SENSE_PIN, GPIO_IN);
    adc_init();
    adc_gpio_init(BATTERY_VOLTAGE_PIN);
    adc_select_input(BATTERY_VOLTAGE_PIN - 26);

    InitTensorflow();
    dp.Init();
    dp.WriteSmallImage(loading_image, 0, 0, 320, 240);
    gpio_put(TFT_BACKLIGHT_PIN, 1);
    dp.disable_writing = true;
    FileReader::Mount();
    
    tf.SetDisplayHandler(display_handler);
    tf.SetFont(FreeSansOblique24pt7b, 0);
    kr.SetFontColour(0);
    kr.OpenFile("64x64.bruh", 64, 64);
    is.SetFontColour(0);
    is.OpenFile("kanji-info.bruh", 320, 240);

    clear();

    GetNMostLikely(drawing_area, guesses, 6);

    dp.disable_writing = false;

    render_results();
    init_draw_screen();

    //gpio_put(TFT_BACKLIGHT_PIN, 1);

    cli_reboot();
    update_watchdog();
    AddTask(power_off, 1000);
    AddTask(sleep_task, 1000);
    while(1) {
        if (screen == 0)
            update_draw_screen();
        else if (screen == 1)
            update_info_screen();
        else if (screen == 2)
            update_status_screen();
        UpdateTasks();
    }
}