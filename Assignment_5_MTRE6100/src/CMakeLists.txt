
# Minimum version used
cmake_minimum_required(VERSION "3.17")

#name of the project (Global Project Name)
project(assignment_5)

# C++ Compiler Version
set(CMAKE_CXX_STANDARD 14)

# Require the OpenCV files
find_package( OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories("../headers")

add_executable(assignment_5.exe main.cpp)

target_link_libraries(assignment_5.exe ${OpenCV_LIBS})



