cmake_minimum_required(VERSION 3.12)

# funções de build do pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# PROJECT_NAME é stepper_pio
project(stepper_pio C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# cria subdiretório pico-sdk no projeto para bibliotecas
pico_sdk_init()

# configura biblioteca do A*
include_directories(astar-lib)
add_library(a-star astar-lib/AStar.cpp)

add_executable(${PROJECT_NAME}
    main.cpp
)

# cria header com nome <programa>.pio.h
pico_generate_pio_header(${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/stepper.pio
)

# cria arquivos .map, .bin, .hex e .uf2 além do .elf
pico_add_extra_outputs(${PROJECT_NAME})

# linka com pico_stdlib
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_pio
    a-star
)

# seleciona USB ao invés de UART
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)