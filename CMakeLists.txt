cmake_minimum_required(VERSION 3.10.0)
project(shi VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)

add_executable(shi main.cc)
target_include_directories(shi PRIVATE include ${Boost_INCLUDE_DIRS})
target_link_libraries(shi PRIVATE OpenSSL::Crypto ZLIB::ZLIB Boost::filesystem)
