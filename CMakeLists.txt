cmake_minimum_required(VERSION 3.21)

include(pico_sdk_import.cmake)

project(kanjis C CXX ASM) # Change to the project name (Does not need to match or anything)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    main.cpp
    ILI9341Driver.cpp
    DataProcessing.cpp
    ImageClassification.cpp
    GFXFontRenderer.cpp
    FileReader.cpp
    KanjiRenderer.cpp
    hw_config.c
    TaskScheduler.cpp
    ProgressBar.cpp
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_malloc
    pico_multicore
    pico_bootrom
    hardware_watchdog
    hardware_gpio
    hardware_vreg
    hardware_clocks
    hardware_spi
    hardware_dma
    hardware_irq
    hardware_adc
    pico-tflmicro
    FatFs_SPI
)

add_subdirectory(pico-tflmicro)
add_subdirectory(no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})

pico_define_boot_stage2(pico-coremark-div4 ${PICO_DEFAULT_BOOT_STAGE2_FILE})     # three lines required for overclock of over 266 MHz
target_compile_definitions(pico-coremark-div4 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
pico_set_boot_stage2(${PROJECT_NAME} pico-coremark-div4)