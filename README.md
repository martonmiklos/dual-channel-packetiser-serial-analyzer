# Dual channel Saleae Asynchronous Serial Analyzer with packet detection capabilities

## What's this?

This branch contains a heavily modified Saleae Asynchronous Serial Analyzer plugin which is equipped with the following capabilities:

- It is capable to decode two UART channels simultenaously
- The decoded bytes (if they follow each other) within a gap tied to packets
- The packets start and end is marked on the view
- The decoded packets from the two channels could be exported to a single file. Example output:
```
0.427849830000000 TX AA 12 01 D0 02 00 F4 C0 0C 00 00 00 00 00 00 00 00 00 00 00 00 CB
0.478017128000000 TX AA 12 01 D0 02 00 F5 C0 0C 00 00 00 00 00 00 00 00 00 00 00 00 4B
0.519377574000000 RX AA 0A 01 D3 04 00 00 00 00 D0 02 00 C2 E6
0.528160586000000 TX AA 12 01 D0 02 00 F6 C0 0C 00 00 00 00 00 00 00 00 00 00 00 00 D2
0.578300896000000 TX AA 12 01 D0 02 00 F7 C0 0C 00 00 00 00 00 00 00 00 00 00 00 00 52
0.596686188000000 TX AA 0A 01 D0 02 00 F8 D3 04 00 00 00 00 51
0.598837142000000 RX AA 2D 00 FA 81 35 38 36 33 39 3A 09 52 78 20 68 65 61 72 74 62 65 61 74 20 35 38 36 33 39 2C 20 69 6E 74 65 72 76 61 6C 20 30 78 30 73 0D 0A 5E
0.628440674000000 TX AA 12 01 D0 02 00 F9 C0 0C 00 00 00 00 00 00 00 00 00 00 00 00 1D
0.678653890000000 TX AA 12 01 D0 02 00 FA C0 0C 00 00 00 00 00 00 00 00 00 00 00 00 84
```

## Getting Started

### MacOS

Dependencies:
- XCode with command line tools
- CMake 3.11+

Installing command line tools after XCode is installed:
```
xcode-select --install
```

Then open XCode, open Preferences from the main menu, go to locations, and select the only option under 'Command line tools'.

Installing CMake on MacOS:

1. Download the binary distribution for MacOS, `cmake-*-Darwin-x86_64.dmg`
2. Install the usual way by dragging into applications.
3. Open a terminal and run the following:
```
/Applications/CMake.app/Contents/bin/cmake-gui --install
```
*Note: Errors may occur if older versions of CMake are installed.*

Building the analyzer:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### Ubuntu 16.04

Dependencies:
- CMake 3.11+
- gcc 4.8+

Misc dependencies:

```
sudo apt-get install build-essential
```

Building the analyzer:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### Windows

Dependencies:
- Visual Studio 2015 Update 3
- CMake 3.11+

**Visual Studio 2015**

*Note - newer versions of Visual Studio should be fine.*

Setup options:
- Programming Languages > Visual C++ > select all sub-components.

Note - if CMake has any problems with the MSVC compiler, it's likely a component is missing.

**CMake**

Download and install the latest CMake release here.
https://cmake.org/download/

Building the analyzer:
```
mkdir build
cd build
cmake ..
```

Then, open the newly created solution file located here: `build\serial_analyzer.sln`
