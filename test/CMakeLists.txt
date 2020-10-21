cmake_minimum_required(VERSION 3.18)

set (CMAKE_CXX_STANDARD 17)

project(tests)
find_package(verilator HINTS $ENV{VERILATOR_ROOT})


# Game State
add_executable(Vadder tb/adder.cpp)

target_include_directories(Vadder PRIVATE ../include)
verilate(Vadder
        TRACE
        SOURCES rtl/adder.sv
        INCLUDE_DIRS rtl tb)