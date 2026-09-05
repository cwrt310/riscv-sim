cmake_minimum_required(VERSION 3.16)
project(riscv-sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)                     # Qt 信号槽需要

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(riscv-sim
    main.cpp
    CPU.cpp
    Memory.cpp
    RegisterFile.cpp
    Assembler.cpp
    Assembler.h
    common.h CPU.h Memory.h
    mainwindow.h mainwindow.cpp mainwindow.ui
)

target_link_libraries(riscv-sim PRIVATE Qt6::Widgets)