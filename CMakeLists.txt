
cmake_minimum_required(VERSION 3.22)
project(embedded-vehicle-stack)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiodcxx)

find_path(TURBOJPEG_INCLUDE_DIR
    NAMES turbojpeg.h
)

find_library(TURBOJPEG_LIBRARY
    NAMES turbojpeg
    REQUIRED
)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        "src/*.cpp"
)

add_executable(embedded-vehicle-stack ${SRC_FILES})

target_include_directories(embedded-vehicle-stack PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        /usr/include/libcamera
        ${TURBOJPEG_INCLUDE_DIR}
)

target_link_libraries(embedded-vehicle-stack PRIVATE
        PkgConfig::GPIOD
        camera
        camera-base
        ${TURBOJPEG_LIBRARY}
)