# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

# Set the project name and supported languages (Added ASM)
project(blink C CXX ASM)

# Initialize the Pico SDK
# Option 1: If PICO_SDK_PATH is set as an environment variable
if(NOT DEFINED PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH is not set. Please set it to the path of your Pico SDK.")
endif()
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Initialize the Pico SDK
pico_sdk_init()

# Add the executable target
add_executable(blink
    blink.c
)

# Link the standard Pico library
target_link_libraries(blink PRIVATE pico_stdlib)

# Optionally link the CYW43 architecture library if supported
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(blink PRIVATE pico_cyw43_arch_none)
endif()

# Enable additional output formats (map, bin, hex, etc.)
pico_add_extra_outputs(blink)

# (Optional) Set the program URL for easy access
# Replace the URL with your project's repository or documentation
# pico_set_program_url(blink "https://github.com/yourusername/yourproject")

# (Optional) Enable USB output, disable UART, etc., as needed
# pico_enable_stdio_usb(blink 1)
# pico_enable_stdio_uart(blink 0)
