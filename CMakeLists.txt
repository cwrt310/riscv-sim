cmake_minimum_required(VERSION 3.10)
project(riscv-sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(riscv-sim
    main.cpp
    CPU.cpp
    Memory.cpp
    RegisterFile.cpp
    Assembler.cpp
)
