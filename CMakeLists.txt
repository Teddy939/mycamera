cmake_minimum_required(VERSION 3.13)
project(MyCamera CXX)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(test)
