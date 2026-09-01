cmake_minimum_required(VERSION 3.10.0)
project(shi VERSION 0.1.0 LANGUAGES C CXX)

add_executable(shi main.cc)
target_include_directories(shi PRIVATE include)
