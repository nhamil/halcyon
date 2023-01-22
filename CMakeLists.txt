cmake_minimum_required(VERSION 3.16) 
project(chess-engine C) 

set(CMAKE_C_STANDARD 11) 

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "--std=c11 -Wall -Wextra -pedantic -m64 -Wno-unused-function -Wno-unused-result -Wno-unused-label")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3 -funroll-loops")

include_directories(source) 

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(sources 
    src/game.c 
    src/search.c 
)

add_executable(halcyon src/main.c ${sources})
target_link_libraries(halcyon Threads::Threads) 

add_executable(perft src/perft.c ${sources})
target_link_libraries(perft Threads::Threads) 