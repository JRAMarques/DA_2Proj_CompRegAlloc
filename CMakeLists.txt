# minimum version required (3.10 is a very safe baseline)
cmake_minimum_required(VERSION 3.10)

project(CompRegAlloc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# turning on standard warnings to keep our code clean and bug-free
add_compile_options(-Wall -Wextra)

# tell CMake where to find our header files (.h or .hpp)
include_directories(include)

# grab all the source files inside the src folder
file(GLOB SOURCES "src/*.cpp")

# create the final executable named 'myProg' as requested in the project specs
add_executable(myProg ${SOURCES})