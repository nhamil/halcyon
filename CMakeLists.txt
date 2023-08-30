cmake_minimum_required(VERSION 3.16) 
project(halcyon C) 

set(ENGINE_VERSION "Dev")

find_package(Git REQUIRED)
if(ENGINE_VERSION STREQUAL "Dev")
    execute_process(
	    COMMAND git log -1 --format=%h 
	    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} 
	    OUTPUT_VARIABLE GIT_HASH 
	    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(ENGINE_VERSION "${ENGINE_VERSION} ${GIT_HASH}")
endif()

string(REGEX REPLACE " $" "" ENGINE_VERSION ${ENGINE_VERSION})
string(TOLOWER ${ENGINE_VERSION} EXE_VERSION)
string(REPLACE " " "" EXE_VERSION ${EXE_VERSION})
add_compile_definitions(ENGINE_NAME="Halcyon ${ENGINE_VERSION}")

message(STATUS "Building Halcyon ${ENGINE_VERSION}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# does not affect build flags: used for naming files
if(NOT EXE_OS)
    if(LINUX) 
        message(STATUS "Building for Linux") 
        set(EXE_OS "linux")
    elseif(WIN32) 
        message(STATUS "Building for Windows") 
        set(EXE_OS "windows")
    elseif(APPLE) 
        message(STATUS "Building for macOS") 
        set(EXE_OS "macos")
    else() 
        message(WARNING "Unknown operating system") 
        set(EXE_OS "unknown")
    endif() 
else() 
    message(STATUS "Override OS: ${EXE_OS}")
endif()
string(TOLOWER ${EXE_OS} EXE_OS)
set(EXE_OS "-${EXE_OS}")

# does not affect build flags: used for naming files
if(NOT EXE_ARCH) 
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} EXE_ARCH)
    if(EXE_ARCH MATCHES "x64|x86_64|amd64")
        set(EXE_ARCH "x64")
        message(STATUS "Building for " ${EXE_ARCH})
    elseif(EXE_ARCH MATCHES "x86")
        set(EXE_ARCH "x86")
        message(STATUS "Building for " ${EXE_ARCH})
    elseif(EXE_ARCH MATCHES "aarch64|arm64|armv[8-9]")
        set(EXE_ARCH "arm64")
        message(STATUS "Building for " ${EXE_ARCH})
    elseif(EXE_ARCH MATCHES "aarch32|arm32|armv[1-7]")
        set(EXE_ARCH "arm32")
        message(STATUS "Building for " ${EXE_ARCH})
    else()
        message(WARNING "Unknown architecture: " ${CMAKE_SYSTEM_PROCESSOR})
        set(EXE_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif()
else()
    message(STATUS "Override architecture: " ${EXE_ARCH})
endif()
string(TOLOWER ${EXE_ARCH} EXE_ARCH)
set(EXE_ARCH "-${EXE_ARCH}")

set(CMAKE_C_STANDARD 99) 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat -Wformat-signedness -pedantic -Wno-unused-function -Wno-unused-result -Wno-unused-label")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -funroll-loops")

# affects build flags
string(TOLOWER "${CPU_EXT}" CPU_EXT)
if((CPU_EXT STREQUAL "") OR (CPU_EXT STREQUAL "none")) 
    message(STATUS "Building without special instruction set")
elseif(CPU_EXT STREQUAL "popcnt")
    add_definitions(-DPOPCNT)
    message(STATUS "Building for popcnt")
    set(EXE_ARCH "${EXE_ARCH}-popcnt")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpopcnt")
elseif(CPU_EXT STREQUAL "bmi2")
    add_definitions(-DPOPCNT -DBMI2)
    message(STATUS "Building for bmi2")
    set(EXE_ARCH "${EXE_ARCH}-bmi2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpopcnt -mbmi2")
elseif(CPU_EXT STREQUAL "avx2")
    add_definitions(-DPOPCNT -DBMI2 -DAVX2)
    message(STATUS "Building for avx2")
    set(EXE_ARCH "${EXE_ARCH}-avx2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpopcnt -mbmi2 -mavx2")
elseif(CPU_EXT STREQUAL "avx512")
    add_definitions(-DPOPCNT -DBMI2 -DAVX2 -DAVX512)
    message(STATUS "Building for avx512")
    set(EXE_ARCH "${EXE_ARCH}-avx512")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpopcnt -mbmi2 -mavx2 -mavx512f")
else() 
    message(WARNING "Unknown instruction set: " ${CPU_EXT})
endif() 

set(EXE_NAME "halcyon-${EXE_VERSION}${EXE_OS}${EXE_ARCH}")
message(STATUS "Executable is " ${EXE_NAME})

include_directories(Source) 

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SOURCES 
    # Source/Eval.c 
    Source/Fen.c 
    Source/Game.c 
    Source/Magic.c 
    Source/MoveGen.c 
    # Source/Search.c 
    # Source/TTable.c 
    Source/Tables.c 
    Source/Zobrist.c
)

add_executable(engine Source/Main.c ${SOURCES})
set_target_properties(engine PROPERTIES OUTPUT_NAME "${EXE_NAME}")

add_executable(engine-valid EXCLUDE_FROM_ALL Source/Main.c ${SOURCES})
set_target_properties(engine-valid PROPERTIES OUTPUT_NAME "${EXE_NAME}-valid")
target_compile_definitions(engine-valid PRIVATE VALIDATION=1)

add_executable(perft EXCLUDE_FROM_ALL Source/Perft.c ${SOURCES})
set_target_properties(perft PROPERTIES OUTPUT_NAME "perft${EXE_ARCH}")

add_executable(perft-valid EXCLUDE_FROM_ALL Source/Perft.c ${SOURCES})
target_compile_definitions(perft-valid PRIVATE VALIDATION=1)
set_target_properties(perft-valid PROPERTIES OUTPUT_NAME "perft${EXE_ARCH}-valid")

add_executable(tune EXCLUDE_FROM_ALL Source/Tune.c ${SOURCES})

add_executable(stabilize EXCLUDE_FROM_ALL Source/Stabilize.c ${SOURCES})

if(STATIC_THREADS)
    target_link_libraries(engine -static Threads::Threads) 
    target_link_libraries(engine-valid -static Threads::Threads) 
    target_link_libraries(perft -static Threads::Threads)
    target_link_libraries(perft-valid -static Threads::Threads)
    target_link_libraries(tune -static Threads::Threads m)
    target_link_libraries(stabilize -static Threads::Threads)
else()
    target_link_libraries(engine Threads::Threads) 
    target_link_libraries(engine-valid Threads::Threads) 
    target_link_libraries(perft Threads::Threads)
    target_link_libraries(perft-valid Threads::Threads)
    target_link_libraries(tune Threads::Threads m)
    target_link_libraries(stabilize Threads::Threads)
endif()
