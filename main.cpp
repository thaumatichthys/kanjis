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
#include "ProjectiveTransform.h"

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

bool erasing = false;

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

uint8_t screen = 0; // 0 == draw, 1 == info, 2 == status, 3 == calibration

void init_info_screen();
void update_info_screen();
void init_status_screen();
void update_status_screen();
void init_calibrate_screen();
void update_calibrate_screen();

void init_draw_screen() {
    dp.FillSmallArea(0, 320, 0, 240, 0xFFFF); // background
    dp.FillSmallArea(192, 320, 0, 47, 0b1000010000010000); // run button
    dp.FillSmallArea(0, 48, 0, 47, dp.RGBto16bit(127, 150, 127)); // clear button
    if (erasing)
        dp.FillSmallArea(49, 96, 0, 47, dp.RGBto16bit(200, 2, 20)); // erase button
    else
        dp.FillSmallArea(49, 96, 0, 47, dp.RGBto16bit(200, 200, 200)); // erase button
    dp.FillSmallArea(97, 144, 0, 47, dp.RGBto16bit(127, 150, 127)); // status screen button
    dp.FillSmallArea(145, 192, 0, 47, dp.RGBto16bit(50, 150, 250)); // calibration button
    
    visual_clear();
    render_current();
    for (int x = 0; x < 192; x++) {
        for (int y = 0; y < 192; y++) {
            if (read_visual_buffer(x, y))
                dp.FillSmallArea(x, x, y + 48, y + 48, 0xFFFF);
        }
    }
    ready = false;
    compute_results();
    sleep_ms(300);
}

bool eraseToggleEnable = true;

void erase_toggle_handler() {
    if (eraseToggleEnable) {
        erasing = !erasing;
        if (erasing) {
            // draw the button for erasing mode
            dp.FillSmallArea(49, 96, 0, 47, dp.RGBto16bit(200, 2, 20)); // erase button
        }
        else {
            // draw the button for not erasing mode
            dp.FillSmallArea(49, 96, 0, 47, dp.RGBto16bit(200, 200, 200)); // erase button
        }
        eraseToggleEnable = false;
        AddTask([]() {
            eraseToggleEnable = true;
        }, 1000);
    }
}

void update_draw_screen() {
    uint16_t x;
    uint16_t y;
    if (dp.ReadTouch(&x, &y)) {
        if (x < 192 - 1 && x > 1 && y > 48 + 1 && y < 240 - 1) {
            if (erasing && x < 192 - 4 && x > 4 && y > 48 + 4 && y < 240 - 4) {
                uint8_t A = 64 - (y - 48) / 3 - 1;
                uint8_t B = x / 3;
                for (int i = -1; i < 2; i++) {
                    for (int j = -1; j < 2; j++) {
                        drawing_area[A + i][B + j]  = -128;
                    }
                }
                for (int i = -4; i < 5; i++) {
                    for (int j = -4; j < 5; j++) {
                        write_visual_buffer(false, x + i, y - 48 + j);
                    }
                }
                dp.FillSmallArea(x - 4, x + 4, y - 4, y + 4, 0);
            }
            else {
                drawing_area[64 - (y - 48) / 3 - 1][x / 3] = 127;
                dp.FillSmallArea(x - 1, x + 1, y - 1, y + 1, 0xFFFF);
                for (int i = -1; i < 2; i++) {
                    for (int j = -1; j < 2; j++) {
                        write_visual_buffer(true, x + i, y - 48 + j);
                    }
                }
            }

            ready = false;
            compute_results();
        }
        else if (x < 48 && y < 48) {
            clear();
            clear_visual_buffer();
            compute_results();
        }
        else if (x >= 48 && x < 96 && y < 48) {
            // ERASE BUTTON
            erase_toggle_handler();
        }
        else if (x >= 96 && x < 144 && y < 48) {
            screen = 2;
            init_status_screen();
        }
        else if (x >= 144 && x < 192 && y < 48) {
            // CALIBRATION SCREEN
            screen = 3;
            init_calibrate_screen();
        }
        else if (x >= 192 && y < 48) {
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

ProgressBar bl(30, 30);
uint32_t blx, bly;
uint16_t blc;
ProgressBar tl(30, 209);
uint32_t tlx, tly;
uint16_t tlc;
ProgressBar br(289, 30);
uint32_t brx, bry;
uint16_t brc;
ProgressBar tr(289, 209);
uint32_t trx, try_; // cant be 'try'
uint16_t trc;
uint8_t sample_finish = 0;

const uint16_t n_samples = 100;
const uint16_t calibrate_progress_colour = 123;
const uint16_t calibrate_done_colour = 234;

int samples = 0;

void init_calibrate_screen() {
    //dp.WriteSmallImage(calibration_image, 0, 0, 320, 240);
    dp.WriteCompressedImage(calibration_image_compressed);
    sleep_ms(300);
    blx = 0; bly = 0; blc = 0;
    tlx = 0; tly = 0; tlc = 0;
    brx = 0; bry = 0; brc = 0;
    trx = 0; try_ = 0; trc = 0;
    sample_finish = 0;
    bl.Stop();
    tl.Stop();
    br.Stop();
    tr.Stop();
}

void update_calibrate_screen() {
    uint16_t x;
    uint16_t y;
    if (dp.ReadTouch(&x, &y)) {
        if (x > 119 && x < 200 && y > 208) {
            // exit 
            screen = 0;
            init_draw_screen();
        }
    }
    if (dp.ReadTouchRaw(&x, &y)) {
        if (x < 80 && y < 64) {
            // bottom left
            bl.Change((float)blc / (float)n_samples, calibrate_progress_colour);
            if (blc <= n_samples) {
                blx += x;
                bly += y;
                blc++;
            }
            else {
                bl.Stop();
                bl.Change(1.0f, 0);
                sample_finish |= 0b00000001;
            }
        }
        else if (x < 80 && y > 175) {
            // top left
            tl.Change((float)tlc / (float)n_samples, calibrate_progress_colour);
            if (tlc <= n_samples) {
                tlx += x;
                tly += y;
                tlc++;
            }
            else {
                tl.Stop();
                tl.Change(1.0f, calibrate_done_colour);
                sample_finish |= 0b00000010;
            }
        }
        else if (x > 239 && y < 64) {
            // bottom right
            br.Change((float)brc / (float)n_samples, calibrate_progress_colour);
            if (brc <= n_samples) {
                brx += x;
                bry += y;
                brc++;
            }
            else {
                br.Stop();
                br.Change(1.0f, calibrate_done_colour);
                sample_finish |= 0b00000100;
            }
        }
        else if (x > 239 && y > 175) {
            // top right
            tr.Change((float)trc / (float)n_samples, calibrate_progress_colour);
            if (trc <= n_samples) {
                trx += x;
                try_ += y;
                trc++;
            }
            else {
                tr.Stop();
                tr.Change(1.0f, calibrate_done_colour);
                sample_finish |= 0b00001000;
            }
        }
    }
    if (sample_finish == 15) {
        // run calibration
        blx /= n_samples;
        bly /= n_samples;
        tlx /= n_samples;
        tly /= n_samples;
        brx /= n_samples;
        bry /= n_samples;
        trx /= n_samples;
        try_ /= n_samples;
        float in1[] = { 
            blx, bly, 
            tlx, tly, 
            brx, bry, 
            trx, try_ 
        };
        float in2[] = { 
            30.0, 30.0f, 
            30.0f, 209.0f,
            289.0f, 30.0f, 
            289.0f, 209.0f
        };
        GetProjectiveTransform(calibration_matrix, in2, in1);
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                printf("%f ", calibration_matrix[a][b]);
            }
            printf("\n");
        }
        sample_finish = 0;
        screen = 0;
        init_draw_screen();
    }
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
    update_status_voltage();
}

void cli_reboot() {
    if (getchar_timeout_us(0) == 'r') {
        reset_usb_boot(0, 0);
    }
    AddTask(cli_reboot, 100);
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
    cli_reboot();

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

        sleep_ms(1000);
        watchdog_update();
        sleep_ms(1000);
        watchdog_update();
        sleep_ms(1000);
        watchdog_update();

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
        else if (screen == 3)
            update_calibrate_screen();
        if (!screen == 0) {
            pb.Stop();
        }
        UpdateTasks();
    }
}