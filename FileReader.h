#pragma once

#include "stdio.h"
#include "pico/stdlib.h"
#include "hw_config.h"  // this is for the SD driver
#include "ff.h"

extern "C" {
    #include "sd_card.h"
}


class FileReader {
    static void throw_error(FRESULT status, const char* comment);
    FIL working_file;
    FILINFO working_file_info;
    public:
    static void Mount();
    void OpenFile(const char* filename);
    void CloseFile();
    void ReadAtOffset(uint32_t byte_offset, uint8_t* output, uint16_t bytes_to_read, uint *bytes_read);
};