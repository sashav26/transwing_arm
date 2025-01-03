# CMakeLists.txt

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(transwingC C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# Add executable
add_executable(transwingC
    transwingC.c
    servo.c
    sbus2.c
)

# Include the current directory for headers
target_include_directories(transwingC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link the Pico libraries
target_link_libraries(transwingC
    pico_stdlib
    hardware_pwm
    hardware_adc
    hardware_uart
)

# Enable USB output, disable UART1 (since UART0 is used for SBUS)
pico_enable_stdio_usb(transwingC 1)
pico_enable_stdio_uart(transwingC 0)

# Create map/bin/hex file
pico_add_extra_outputs(transwingC)
