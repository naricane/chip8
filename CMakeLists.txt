cmake_minimum_required(VERSION 3.10)

project(chip8)

set(SOURCES
  src/chip8.c
  src/error.c
  src/main.c
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(chip8 ${SOURCES} )

target_link_libraries(chip8 ${SDL2_LIBRARIES} m)
