cmake_minimum_required(VERSION 3.20)
project(simple-library VERSION 1.0.0)

add_library(calculator STATIC src/calculator.cpp)

target_include_directories(calculator PUBLIC include)
