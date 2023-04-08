#pragma once

#include "stdio.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "math.h"


// default values
#define MISO    16
#define TFT_CS  15 // CS for the display
#define XPT_CS  22 // CS for the touch controller
#define SCLK    18
#define MOSI    19
#define TFT_RST 20
#define TFT_DC  21
#define TFT_BITRATE 40000000   // adjust to your needs
#define XPT_BITRATE 500000    // the touch controller doesn't like 40MHz baud :/
#define SPI_PORT spi0

/*
    Methods of operation
    for ILI9341:
        Startup:
            Send all the required commands and parameters for startup.
        Writing to the display:
            Send the column and row address select commands (8 bits each), and send the bounds as parameters (16 bits for each number).
            Send the memory write command (8 bits), and send the data as parameters (16 bits per pixel).
    for XPT2046:
        Startup does not require any commands.
        Get values: 
            Send a control byte, and read the data out afterwards.

*/

// these were obtained from here https://github.com/adafruit/Adafruit_ILI9341
const uint8_t startup_cmds[] = {
    0xEF, 3, 0x03, 0x80, 0x02,
    0xCF, 3, 0x00, 0xC1, 0x30,
    0xED, 4, 0x64, 0x03, 0x12, 0x81,
    0xE8, 3, 0x85, 0x00, 0x78,
    0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
    0xF7, 1, 0x20,
    0xEA, 2, 0x00, 0x00,
    0xC0, 1, 0x23, // "power control 1"
    0xC1, 1, 0x10, // "power control 2"
    0xC5, 2, 0x3E, 0x28, // "vcm control"
    0xC7, 1, 0x86, // "vcm control 2"
    0x36, 1, 0x48, // "memory access control"
    0x37, 1, 0x00, // "vertical scroll zero"
    0x3A, 1, 0x55, // "pixel format set"
    0xB1, 2, 0x00, 0x18, // "frame rate control"
    0xB6, 3, 0x08, 0x82, 0x27, // "display function control"
    0xF2, 1, 0x00, // "3gamma function disable"
    0x26, 1, 0x01, // "gamma curve selected"
    0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
    0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    0x11, 0x80, // "exit sleep" (the 0x80 is not supposed to actually be sent)
    0x29, 0x80, // "display on"
    0x00
};

class ILI9341 {
    const uint16_t dx = 320;
    const uint16_t dy = 240;
    static uint32_t tft_baudrate;
    static uint32_t xpt_baudrate;
    static uint16_t pixel_colour; // this needs to be here since local values get deleted after the function returns, so the DMA can't read those
    static uint8_t miso;
    static uint8_t tft_cs;
    static uint8_t xpt_cs;
    static uint8_t sclk;
    static uint8_t mosi;
    static uint8_t tft_rst;
    static uint8_t tft_dc;
    static spi_inst_t* spi_port;

    static bool prev_detected;
    static uint16_t prev_x;
    static uint16_t prev_y;

    void Write16(uint16_t value);

    static void SetPinState(uint8_t pin, bool state);
    static void CPUDelay();

    public:
    void Init();
    uint16_t RGBto16bit(uint8_t r, uint8_t g, uint8_t b);
    void FillSmallArea(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye, uint16_t colour);
    void WritePixel(uint16_t x, uint16_t y, uint16_t colour);
    void WriteSmallImage(const uint16_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    bool ReadTouch(uint16_t* x, uint16_t* y);
    //void CalibrateTouchBlocking();
    void CorrectValues(uint16_t* x, uint16_t* y, const float coefficients[3][3]);
    void RenderBinary(const uint8_t* img, uint16_t colour, uint16_t background, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
};

static const float calibration_matrix[3][3] = {
    {1.1431899240291263, 0.006698261801041863, -17.979144411907356}, 
    {0.03085493403284114, 1.0973331794214367, -20.534946776364155}, 
    {-4.365957944118304e-05, 2.2327539336806273e-05, 1.0}
};
