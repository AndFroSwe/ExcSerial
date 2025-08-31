# ExcSerial

Simple windows CLI application to send alternating
serial output to a COM-port.

The initial purpose was to test send data to MCU
controlling motors in a simple way (project name Excelsior,
therefore ExcSerial).

# Build

Simple. Download library and compile with MSVC, clang-cl
or MinGW. Note that there is no getting of MinGW DLLs.

Set CMAKE_INSTALL_PREFIX to desired value and install.

# Usage

Run from command line:

```
$ excserial COM3 15 250
```

sends message "#15,15,15,15,;" to COM3 at 250 Hz.
alternating between 15 and -15.

# Project info

- Author: Andreas Fröderberg