cmake_minimum_required(VERSION 3.20)

project(shi
    VERSION 0.1.0
    LANGUAGES CXX
)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Homebrew is searched automatically on macOS when installed in the default
# location. CMAKE_PREFIX_PATH can be used for other installations, e.g.:
# cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(ZLIB REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem process)
find_library(SSH_LIBRARY NAMES ssh REQUIRED)

add_executable(shi
    main.cc
)

target_include_directories(shi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

target_compile_options(shi PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(shi PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
    Boost::filesystem
    Boost::process
    ${SSH_LIBRARY}
)

set_target_properties(shi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
