#include "ILI9341Driver.h"


// I should probably add a setter for all these values
uint16_t ILI9341::pixel_colour;
uint32_t ILI9341::tft_baudrate = TFT_BITRATE;
uint32_t ILI9341::xpt_baudrate = XPT_BITRATE;
uint8_t ILI9341::miso = MISO;
uint8_t ILI9341::tft_cs = TFT_CS;
uint8_t ILI9341::xpt_cs = XPT_CS;
uint8_t ILI9341::sclk = SCLK;
uint8_t ILI9341::mosi = MOSI;
uint8_t ILI9341::tft_rst = TFT_RST;
uint8_t ILI9341::tft_dc = TFT_DC;
spi_inst_t* ILI9341::spi_port = SPI_PORT;

bool ILI9341::prev_detected = false;
uint16_t ILI9341::prev_x;
uint16_t ILI9341::prev_y;

bool ILI9341::disable_writing = false;

void ILI9341::CPUDelay() {
    volatile uint8_t dummy = 0;
    for (uint32_t i = 0; i < (2000) / ILI9341::tft_baudrate; i++) { dummy = 123; }
}

void ILI9341::SetPinState(uint8_t pin, bool state) {
    gpio_put(pin, state);
}

void ILI9341::Write16(uint16_t value) {
    uint8_t arr[2] = {(uint8_t) (value >> 8), (uint8_t) value};
    spi_write_blocking(ILI9341::spi_port, arr, 2);
}


void ILI9341::Init() {
    spi_init(this->spi_port, ILI9341::tft_baudrate);
    gpio_set_function(ILI9341::miso, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341::sclk, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341::mosi, GPIO_FUNC_SPI);
    gpio_init(ILI9341::tft_cs);
    gpio_set_dir(ILI9341::tft_cs, GPIO_OUT);
    ILI9341::SetPinState(ILI9341::tft_cs, 1); // CS is active low
    gpio_init(ILI9341::xpt_cs);
    gpio_set_dir(ILI9341::xpt_cs, GPIO_OUT);
    ILI9341::SetPinState(ILI9341::xpt_cs, 1);
    gpio_init(ILI9341::tft_rst);
    gpio_init(ILI9341::tft_dc);
    gpio_set_dir(ILI9341::tft_rst, GPIO_OUT);
    gpio_set_dir(ILI9341::tft_dc, GPIO_OUT);
    ILI9341::SetPinState(ILI9341::tft_rst, 1);
    int iterations = 2;
    size_t startup_cmds_size = sizeof(startup_cmds);
    sleep_ms(100);
    ILI9341::SetPinState(ILI9341::tft_cs, 0);
    for (int i = 0; i < iterations; i++) {
        if (i + 1 == iterations) {
            if (startup_cmds_size < i) {
                return;
            }
            uint8_t n = startup_cmds[i];
            uint8_t command = startup_cmds[i - 1];
            ILI9341::SetPinState(ILI9341::tft_dc, 0);
            spi_write_blocking(ILI9341::spi_port, &command, 1);
            ILI9341::SetPinState(ILI9341::tft_dc, 1);
            if (n == 0x80) {
                sleep_ms(150);
                iterations += 2;
            }
            else {
                for (int o = 0; o < n; o++) {
                    spi_write_blocking(ILI9341::spi_port, &startup_cmds[o + i + 1], 1);
                }
                iterations += n + 2;
            }
        }
    }
    ILI9341::SetPinState(ILI9341::tft_cs, 1);
}

uint16_t ILI9341::RGBto16bit(uint8_t r, uint8_t g, uint8_t b) {
    r >>= 3;
    g >>= 2;
    b >>= 3;
    return (((uint16_t) r & 31) << 11) | (((uint16_t) g & 63) << 5) | ((uint16_t) b & 31);
}

void ILI9341::FillSmallArea(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye, uint16_t colour) {
    if (ILI9341::disable_writing) return;
    ILI9341::SetPinState(ILI9341::tft_cs, 0);
    const uint32_t area = (abs(xs - xe) + 1) * (abs(ys - ye) + 1);
    const uint8_t column_address_set = 0x2A;
    const uint8_t page_address_set = 0x2B;
    const uint8_t memory_write = 0x2C;

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &page_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(xs);
    this->Write16(xe);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &column_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(ys);
    this->Write16(ye);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &memory_write, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);

    for (int i = 0; i < area; i++) {
        this->Write16(colour);
    }
    ILI9341::SetPinState(ILI9341::tft_cs, 1);
}

void ILI9341::WritePixel(uint16_t x, uint16_t y, uint16_t colour) {
    if (ILI9341::disable_writing) return;
    ILI9341::SetPinState(ILI9341::tft_cs, 0);
    const uint8_t column_address_set = 0x2A;
    const uint8_t page_address_set = 0x2B;
    const uint8_t memory_write = 0x2C;

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &page_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(x);
    this->Write16(x);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &column_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(y);
    this->Write16(y);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &memory_write, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);

    this->Write16(colour);
    ILI9341::SetPinState(ILI9341::tft_cs, 1);
}

/* 
import cv2

img = cv2.imread("X:/zkanjidevice-imgs/blanktft.png")
DISPLAY_WIDTH = img.shape[1]    # 320
DISPLAY_HEIGHT = img.shape[0]   # 240

output_str = "const uint16_t image[] = {\n    "
n_elements = 0

for x in range(DISPLAY_WIDTH):
    for y in range(DISPLAY_HEIGHT):
        r = img[DISPLAY_HEIGHT - y - 1, x, 0] >> 3
        g = img[DISPLAY_HEIGHT - y - 1, x, 1] >> 2
        b = img[DISPLAY_HEIGHT - y - 1, x, 2] >> 3
        value = ((b & 31) << 11) | ((g & 63) << 5) | (r & 31)
        output_str += str(value) + ", "
        n_elements += 1
        if n_elements % 121 == 0:
            output_str += "\n    "

output_str += "\n};\n\n/" + "* Program which generated this file:\n"
output_str += open("main.py").read() + "\n*" + "/\n"
print(output_str)

if DISPLAY_HEIGHT != 240 or DISPLAY_WIDTH != 320:
    print("// Your image is not 320x240, ignore if you meant to do this")
*/

void ILI9341::WriteSmallImage(const uint16_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    if (ILI9341::disable_writing) return;
    ILI9341::SetPinState(ILI9341::tft_cs, 0);

    const uint8_t column_address_set = 0x2A;
    const uint8_t page_address_set = 0x2B;
    const uint8_t memory_write = 0x2C;

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &column_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(y);
    this->Write16(y + h - 1);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &page_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(x);
    this->Write16(x + w);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &memory_write, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);

    for (int i = 0; i < w * h; i++) {
        this->Write16(img[i]);
    }

    ILI9341::SetPinState(ILI9341::tft_cs, 1);
}

void ILI9341::WriteCompressedImage(const uint32_t* img) {
    if (ILI9341::disable_writing) return;
    ILI9341::SetPinState(ILI9341::tft_cs, 0);

    const uint8_t column_address_set = 0x2A;
    const uint8_t page_address_set = 0x2B;
    const uint8_t memory_write = 0x2C;

    const uint16_t x = 0;
    const uint16_t y = 0;
    const uint16_t w = 320;
    const uint16_t h = 240;

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &column_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(y);
    this->Write16(y + h - 1);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &page_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(x);
    this->Write16(x + w);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &memory_write, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);

    for (int i = 1; i < img[0] - 1; i++) {
        uint16_t repeats = (uint16_t) (img[i] & 0xFFFF);
        uint16_t pixel = (uint16_t) (img[i] >> 16);
        for (int j = 0; j < repeats + 1; j++) {
            this->Write16(pixel);
        }
    }
    
    ILI9341::SetPinState(ILI9341::tft_cs, 1);
}

void ILI9341::RenderBinary(const uint8_t* img, uint16_t colour, uint16_t background, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    if (ILI9341::disable_writing) return;
    ILI9341::SetPinState(ILI9341::tft_cs, 0);

    const uint8_t column_address_set = 0x2A;
    const uint8_t page_address_set = 0x2B;
    const uint8_t memory_write = 0x2C;

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &column_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(y);
    this->Write16(y + h - 1);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &page_address_set, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);
    this->Write16(x);
    this->Write16(x + w);

    ILI9341::SetPinState(ILI9341::tft_dc, 0);
    spi_write_blocking(ILI9341::spi_port, &memory_write, 1);
    ILI9341::SetPinState(ILI9341::tft_dc, 1);

    for (int x_offset = 0; x_offset < w; x_offset++) {
        for (int y_offset = h - 1; y_offset >= 0; y_offset--) {
            int bit_number = x_offset + w * y_offset;
            int bit_offset = bit_number % 8; // (the bit inside the byte)
            int current_file_index = bit_number / 8;
            if (img[current_file_index] & (1U << (8 - bit_offset - 1)))
                this->Write16(colour);
            else
                this->Write16(background);
        }
    }

    ILI9341::SetPinState(ILI9341::tft_cs, 1);
}

bool ILI9341::ReadTouch(uint16_t* x, uint16_t* y) {
    uint16_t x_raw, y_raw;
    bool outcome = this->ReadTouchRaw(&x_raw, &y_raw);
    CorrectValues(&x_raw, &y_raw, calibration_matrix);
    if (x_raw > 320 || y_raw > 240)
        return false;
    *x = x_raw;
    *y = y_raw;
    return outcome;
}

bool ILI9341::ReadTouchRaw(uint16_t* x, uint16_t* y) {
    spi_set_baudrate(ILI9341::spi_port, ILI9341::xpt_baudrate); // the touch controller is quite slow
    ILI9341::SetPinState(ILI9341::xpt_cs, 0);
    //                             1XXX0001  ->  Start | 12 bit | differential | power down between conversions, IRQ disabled
    const uint8_t cmd_template = 0b10000001;
    const uint8_t n_repeats = 6;
    const float error_max_percent = 0.1;
    uint16_t z1 = 0;
    uint16_t z2 = 0;
    uint8_t rbuf[2];
    const uint8_t z1_control_byte = cmd_template | (3U << 4);
    spi_write_blocking(ILI9341::spi_port, &z1_control_byte, 1); // writes the control byte
    spi_read_blocking(ILI9341::spi_port, 0, rbuf, 2); // read the data
    z1 = (((uint16_t) rbuf[0] << 8) | rbuf[1]) >> 3;
    const uint8_t z2_control_byte = cmd_template | (4U << 4);
    spi_write_blocking(ILI9341::spi_port, &z2_control_byte, 1);
    spi_read_blocking(ILI9341::spi_port, 0, rbuf, 2);
    z2 = (((uint16_t) rbuf[0] << 8) | rbuf[1]) >> 3;
    int32_t z = z1 + 4095 - z2;
    if (z < 400) {
        this->prev_detected = false;
        return false;
    }
    uint32_t x_val = 0;
    uint32_t y_val = 0;
    int16_t x_vals_buf[n_repeats];
    int16_t y_vals_buf[n_repeats];
    const uint8_t x_control_byte = cmd_template | (1U << 4);
    const uint8_t y_control_byte = cmd_template | (5U << 4);
    for (uint8_t i = 0; i < n_repeats; i++) {
        spi_write_blocking(ILI9341::spi_port, &x_control_byte, 1);
        spi_read_blocking(ILI9341::spi_port, 0, rbuf, 2);
        x_vals_buf[i] = (((uint16_t) rbuf[0] << 8) | rbuf[1]) >> 3;
        x_val += x_vals_buf[i];
        spi_write_blocking(ILI9341::spi_port, &y_control_byte, 1);
        spi_read_blocking(ILI9341::spi_port, 0, rbuf, 2);
        y_vals_buf[i] = (((uint16_t) rbuf[0] << 8) | rbuf[1]) >> 3;
        y_val += y_vals_buf[i];
    }
    x_val = (x_val / n_repeats);
    y_val = (y_val / n_repeats);
    uint8_t count_x = n_repeats;
    uint8_t count_y = n_repeats;
    // if one value is very different from the average, remove it
    for (uint8_t i = 0; i < n_repeats; i++) {  
        if (abs(((float) x_vals_buf[i] - x_val) / x_val) > error_max_percent) {
            x_vals_buf[i] = -1;
            count_x--;
        }
        if (abs(((float) y_vals_buf[i] - y_val) / y_val) > error_max_percent) {
            y_vals_buf[i] = -1;
            count_y--;
        }
    }
    x_val = 0;
    y_val = 0;
    for (uint8_t i = 0; i < n_repeats; i++) {
        if (x_vals_buf[i] > 0)
            x_val += x_vals_buf[i];
        if (y_vals_buf[i] > 0) 
            y_val += y_vals_buf[i];
    }
    x_val /= count_x;
    y_val /= count_y;
    ILI9341::SetPinState(ILI9341::xpt_cs, 1);
    spi_set_baudrate(ILI9341::spi_port, ILI9341::tft_baudrate);
    uint16_t out_x = this->dx - x_val * this->dx / 4095;
    uint16_t out_y = this->dy - y_val * this->dy / 4095;
    if ((abs((int) out_x - (int) this->prev_x) > 5 || abs((int) out_y - (int) this->prev_y) > 5) && this->prev_detected) {
        this->prev_x = this->dx;
        this->prev_y = this->dy;
        this->prev_detected = false;
        return false;
    }
    *x = out_x;
    *y = out_y;
    this->prev_detected = true;
    return true;
}

void ILI9341::CorrectValues(uint16_t* x, uint16_t* y, const float coefficients[3][3]) {
    // matrix multiply
    /*  coefficients
            a b c       x   output x
            d e f   *   y = output y
            g h i       1
    */
    uint16_t x_val = *x;
    uint16_t y_val = *y;
    uint16_t output_x = (coefficients[0][0] * x_val + coefficients[0][1] * y_val + coefficients[0][2]) / (coefficients[2][0] * x_val + coefficients[2][1] * y_val + 1);
    uint16_t output_y = (coefficients[1][0] * x_val + coefficients[1][1] * y_val + coefficients[1][2]) / (coefficients[2][0] * x_val + coefficients[2][1] * y_val + 1);
    *x = output_x;
    *y = output_y;
}
