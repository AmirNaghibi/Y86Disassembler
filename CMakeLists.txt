cmake_minimum_required(VERSION 3.13)
project(a1_204_f3k0b_h4l0b)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(a1_204_f3k0b_h4l0b
        coverpage.txt
        disassembler.c
        Makefile
        printRoutines.c
        printRoutines.h
        README.md
        sum_64.mem
        sum_64.ys)
