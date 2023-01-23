cmake_minimum_required(VERSION 3.16) 
project(chess-engine C) 

set(EXE_VERSION "0.2")

find_package(Git REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE BRANCH_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
if(BRANCH_NAME STREQUAL "master") 
    set(BRANCH_NAME "")
endif()

set(EXE_VERSION "${EXE_VERSION}${BRANCH_NAME}")
add_compile_definitions(ENGINE_NAME="Halcyon ${EXE_VERSION}")

message(STATUS "Building Halcyon ${EXE_VERSION}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()
if(LINUX) 
    message(STATUS "Building for Linux") 
    set(EXE_OS "-linux")
elseif(WIN32) 
    message(STATUS "Building for Windows") 
    set(EXE_OS "-windows")
else() 
    message(FATAL_ERROR "Unsupported operating system") 
    set(EXE_OS "-unknown")
endif() 

if(CMAKE_SIZEOF_VOID_P EQUAL 4) 
    message(STATUS "Building 32-bit")
    set(BIT_FLAG "-m32")
    set(EXE_ARCH "-x32")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8) 
    message(STATUS "Building 64-bit")
    set(BIT_FLAG "-m64")
    set(EXE_ARCH "-x64")
else() 
    math(EXPR BIT_COUNT "${CMAKE_SIZEOF_VOID_P} * 8")
    message(STATUS "Unknown bit mode: " ${BIT_COUNT})
endif() 

set(CMAKE_C_STANDARD 11) 
set(CMAKE_C_FLAGS "--std=c11 -Wall -Wextra -pedantic ${BIT_FLAG} -Wno-unused-function -Wno-unused-result -Wno-unused-label")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3 -funroll-loops")

set(EXE_NAME "halcyon-${EXE_VERSION}${EXE_OS}${EXE_ARCH}")

include_directories(source) 

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(sources 
    src/game.c 
    src/search.c 
)

add_executable(engine src/main.c ${sources})
target_link_libraries(engine Threads::Threads) 
set_target_properties(engine PROPERTIES OUTPUT_NAME "${EXE_NAME}")

add_executable(perft src/perft.c ${sources})
target_link_libraries(perft Threads::Threads) 