# -----------------------------------------------------------------------------
#  Alvo32Bits.cmake — garante que TODO o projeto é compilado para x86 (32 bits).
#
#  * MSVC:   gerador VS com -A Win32 (forçado no CMakeLists raiz) ou o
#            "x86 Native Tools Command Prompt" com Ninja.
#  * MinGW:  toolchain i686-w64-mingw32 (cmake/toolchains/mingw-i686.cmake).
#  * Linux:  gcc/clang com -m32 (cmake/toolchains/linux-i686.cmake) e as
#            bibliotecas :i386 do X11/OpenGL instaladas.
#
#  As flags -m32 / /MACHINE:X86 vão em CMAKE_<LANG>_FLAGS (via toolchain), então
#  valem também para raylib e GLFW, compilados por FetchContent no mesmo build.
# -----------------------------------------------------------------------------

function(andv_exigir_32_bits)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR
      "Andvarion exige alvo 32 bits (x86/Win32), mas o compilador gera ponteiros de "
      "${CMAKE_SIZEOF_VOID_P} bytes.\n"
      "  Visual Studio : cmake --preset win32-msvc   (ou -A Win32)\n"
      "  MinGW         : cmake --preset mingw-i686\n"
      "  Linux         : cmake --preset linux-i686   (requer gcc-multilib e libs :i386)\n")
  endif()
  message(STATUS "Andvarion: alvo 32 bits confirmado (sizeof(void*) = 4, processador = ${CMAKE_SYSTEM_PROCESSOR})")
endfunction()

# Aplica avisos, define a macro de arquitetura e reforça as flags de 32 bits.
function(andv_aplicar_alvo alvo)
  target_compile_definitions(${alvo} PRIVATE ANDV_ALVO_32_BITS=1)
  if(MSVC)
    target_compile_options(${alvo} PRIVATE /W4 /utf-8 /permissive- /wd4100)
    target_link_options(${alvo} PRIVATE /MACHINE:X86)
    target_compile_definitions(${alvo} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
  else()
    target_compile_options(${alvo} PRIVATE -m32 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
    target_link_options(${alvo} PRIVATE -m32)
    if(MINGW)
      # Deixa o .exe autocontido (sem libgcc_s_dw2-1.dll / libstdc++-6.dll).
      target_link_options(${alvo} PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
  endif()
endfunction()
