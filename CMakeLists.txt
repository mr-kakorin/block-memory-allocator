cmake_minimum_required(VERSION 3.14)
project(block-memory-allocator C)

#set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE "Debug")

set(sourcedir src)
set(sourcefiles ${sourcedir}/*.c)
set(headerfiles ${sourcedir}/*.h)
file(GLOB_RECURSE sources ${sourcefiles})
file(GLOB_RECURSE headers ${headerfiles})


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("debug build")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -Wall -Wno-long-long -pedantic")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Wall -Wno-long-long -pedantic")
else ()
    message("release build")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native -ffast-math -flto")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -ffast-math -flto")
endif ()

add_executable(block-memory-allocator ${sources} ${headers})
