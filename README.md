# ESP32

Uses code from https://github.com/iodeo/Minitel-ESP32
Follow readme inside arduino folder
TLDR 
- Download this as zip => https://github.com/iodeo/Minitel1B_Hard
- Put in arduino library folder
- Download this as zip => https://github.com/ewpa/LibSSH-ESP32
- Put in arduino library folder
- Install board esp32 by espressif version 2.0.17 (not 3.x.x)
- Select board ESP32 Dev Module
- Load Sketch
- * Voilà *


# Linux

Tester sous Debian 12 (bookworm) avec gcc 12.2.0, sous x86_64

``` sh
# Uses vcpkg
# Edit CMakeUserPresets.json before launching
#  to specify your vcpkg path

# To install VCPKG, visit: https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash
cmake --preset default
cmake --build build
```
