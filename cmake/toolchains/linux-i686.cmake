# Linux x86 (32 bits) com gcc -m32.
#   Debian/Ubuntu:
#     sudo dpkg --add-architecture i386 && sudo apt update
#     sudo apt install gcc-multilib g++-multilib \
#          libx11-dev:i386 libxrandr-dev:i386 libxinerama-dev:i386 \
#          libxcursor-dev:i386 libxi-dev:i386 libgl-dev:i386
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_FLAGS_INIT   "-m32")
set(CMAKE_CXX_FLAGS_INIT "-m32")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-m32")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-m32")

# Procura bibliotecas nas pastas de 32 bits antes das de 64.
set(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)
set(CMAKE_IGNORE_PATH /usr/lib/x86_64-linux-gnu /lib/x86_64-linux-gnu)
set(FIND_LIBRARY_USE_LIB32_PATHS ON)
set(CMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX 32)
