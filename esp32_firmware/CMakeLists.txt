# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(ENV{CC}, /Users/nick/.espressif/tools/xtensa-esp32s2-elf/esp-2022r1-11.2.0/xtensa-esp32s2-elf/bin/xtensa-esp32s2-elf-g++)
set(ENV{IDF_PATH} "/Users/nick/esp/esp-idf")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(flowmeter)