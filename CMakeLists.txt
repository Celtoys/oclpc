cmake_minimum_required(VERSION 2.6.2)

project(oclpc)

include_directories(
    .
)

set(CMAKE_CXXFLAGS "${CMAKE_CXXFLAGS}  -g -O0 -Wall")

# Linker options
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS}  ")

# Library path
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L\".\" ")

set(SRCS
    src/oclpc.c
)

add_executable(oclpc ${SRCS})

target_link_libraries(oclpc
    OpenCL
)

