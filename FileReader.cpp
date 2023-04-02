#include "FileReader.h"


FRESULT fr;
FATFS file_system;
DIR working_directory;

void FileReader::throw_error(FRESULT status, const char* comment) {
    while(1) {
        printf("File error: %d. Comment: %s\n", status, comment);
        sleep_ms(1000);
    }
}

void FileReader::Mount() {
    fr = f_mount(&file_system, "", 1);
    if (fr != FR_OK) FileReader::throw_error(fr, "error in mount");
    fr = f_chdir("KanjiDevice");
    if (fr != FR_OK) FileReader::throw_error(fr, "error in chgdir");
}

void FileReader::OpenFile(const char* filename) {
    fr = f_open(&this->working_file, filename, FA_READ);
    if (fr != FR_OK) FileReader::throw_error(fr, "error in opening file");
}

void FileReader::CloseFile() {
    fr = f_close(&this->working_file);
    if (fr != FR_OK) FileReader::throw_error(fr, "error in closing file");
}

void FileReader::ReadAtOffset(uint32_t byte_offset, uint8_t* output, uint16_t bytes_to_read, uint* bytes_read) {
    fr = f_lseek(&this->working_file, (FSIZE_t) byte_offset);
    if (fr != FR_OK) FileReader::throw_error(fr, "error in seek");
    fr = f_read(&this->working_file, output, bytes_to_read, (UINT*) bytes_read);
    if (fr != FR_OK) FileReader::throw_error(fr, "error in read");
    //printf("%s\n", output);
}