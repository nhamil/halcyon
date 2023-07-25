# Halcyon 1

## Usage 

Halcyon is a command line program and is not meant to be used directly by the 
user. Instead add it as an engine in a UCI-supported GUI such as Cute Chess, 
Scid, or Arena. 

The following options are supported: 

| Option   | Description                   | Default | Minimum | Maximum |
| -------- | ----------------------------- | ------- | ------- | ------- |
| Hash     | Size of the hash table in MiB |      16 |       1 |   16384 | 
| Contempt | Contempt factor in centipawns |       0 |   -1000 |    1000 | 

The following UCI commands are supported: 

* `uci`: Tell the engine to use the UCI protocol.
* `ucinewgame`: Sent when the next search is from a different game.
* `isready`: Wait until the engine is ready for input.
* `position`: Sets the current board state.
    * `startpos [moves [move]...]`: Use the starting position and apply moves.
    * `fen <fen> [moves [move]...]`: Use a FEN board state and apply moves.
* `go`: Start evaluating the position.
    * `depth <depth>`: Maximum depth to search. 
    * `movetime <ms>`: How long to search for. 
    * `wtime <ms>`: How much time white has remaining.
    * `btime <ms>`: How much time black has remaining.
    * `winc <ms>`: How much increment white has each turn. 
    * `binc <ms>`: How much increment black has each turn. 
    * `perft <depth>`: Run perft instead of evaluation. 
* `stop`: Stop evaluating the position and return the best move. 
* `setoption`: Sets a customizable option (listed above).
    * `name <name>`: Case-sensitive option name. 
    * `value <value>`: New value for the option. 
* `quit`: Exit the program. 

## Compiling 

This project uses CMake and has only been tested with GCC on Linux and 
MinGW-w64 on Windows. Other compilers are not guaranteed to work. Not all 
combinations have been testing, so it is possible that some operating systems 
and/or architectures may require modifications to the project to compile. 

The following x86 instruction set extension options are available: 
* `NONE`
* `POPCNT`
* `BMI2` 
* `AVX2`
* `AVX512`

If CMake cannot determine your operating system, you can manually assign it 
with `-DEXE_OS=<os>`. CPU architecture can be assigned with 
`-DEXE_ARCH=<arch>`. Neither of these options affect the build and are only for 
file naming. 

### Linux 

To build, do the following: 
```sh
cd Build
cmake .. -DCPU_EXT=<ext>
make
```

### Windows 

To build, do the following: 
```sh
cd Build
cmake .. -G "MinGW Makefiles" -DCPU_EXT=<ext> -DSTATIC_THREADS=1
mingw32-make.exe
```
