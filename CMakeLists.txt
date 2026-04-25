# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.20)
set(PICO_BOARD pico)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(sensors C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# turn on all compiler warnings 
add_compile_options(-Wall -Wextra )

# print memory information
string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}

  examples/bmp280/SPI_Normal/main.cpp 
  #examples/bmp280/SPI_Forced/main.cpp 
  #examples/bmp280/I2C_Normal/main.cpp

  #examples/lm35/main.cpp

  #examples/lm75/basic/main.cpp
  #examples/lm75/test/main.cpp

  #examples/ahtxx/main.cpp

  #examples/dht11/main.cpp
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

add_library(pico_sensors INTERFACE)

target_sources(pico_sensors INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/bmp280/bmp280.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/lm35/lm35.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/lm75/lm75.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ahtxx/ahtxx.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/dht11/dht11.cpp
)

target_include_directories(pico_sensors INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

# Pull in pico libraries that we need
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_adc hardware_spi hardware_i2c pico_sensors )


# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

