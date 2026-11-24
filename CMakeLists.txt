# =============================================================================
#  ANDVARION — O Sussurro sob a Catedral
#  Action RPG gótico top-down em pixel art (C++17 + raylib 5.0)
#
#  ALVO OBRIGATÓRIO: 32 bits (x86 / Win32 / i686).
#  A configuração é recusada se o compilador gerar ponteiros de 8 bytes.
#  Use um dos presets (CMakePresets.json):
#     cmake --preset win32-msvc     && cmake --build --preset win32-msvc-release
#     cmake --preset mingw-i686     && cmake --build --preset mingw-i686-release
#     cmake --preset linux-i686     && cmake --build --preset linux-i686-release
# =============================================================================
cmake_minimum_required(VERSION 3.21)

# Em geradores Visual Studio, força a plataforma Win32 quando ninguém passou -A.
if(CMAKE_GENERATOR MATCHES "Visual Studio" AND NOT CMAKE_GENERATOR_PLATFORM)
  set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "Plataforma do gerador VS (obrigatório: Win32)" FORCE)
endif()

project(Andvarion
  VERSION 0.1.0
  DESCRIPTION "Action RPG gótico top-down em pixel art"
  LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Alvo32Bits)          # trava de arquitetura + flags -m32 / /MACHINE:X86
andv_exigir_32_bits()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ANDV_TESTES        "Compila os testes de lógica (sem janela)" ON)
option(ANDV_GERAR_VFX     "Regenera assets/vfx com o pixelfx (Node 18+) no build" OFF)

# -----------------------------------------------------------------------------
#  Dependências (fixadas por versão). Primeiro tenta o pacote do sistema/vcpkg
#  (precisa ser a variante x86!), senão baixa e compila junto, já em 32 bits.
# -----------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
  set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)
  # O CMakeLists da raylib 5.0 declara cmake_minimum_required(VERSION 3.0),
  # que o CMake 4.x recusa. Isto só afeta o subprojeto baixado.
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  FetchContent_Declare(raylib
    URL      https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    URL_HASH SHA256=98f049b9ea2a9c40a14e4e543eeea1a7ec3090ebdcd329c4ca2cf98bc9793482)
  FetchContent_MakeAvailable(raylib)
endif()

find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d)
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# -----------------------------------------------------------------------------
#  andv_nucleo: lógica pura (sem raylib). Missões, diálogos, progressão,
#  árvore de habilidades, inventário, save, parser de atlas Aseprite.
#  É o que os testes usam, e o que roda igual em qualquer plataforma 32-bit.
# -----------------------------------------------------------------------------
add_library(andv_nucleo STATIC
  src/core/EventBus.cpp
  src/core/Texto.cpp
  src/gfx/AtlasAseprite.cpp
  src/progressao/Progressao.cpp
  src/progressao/ArvoreHabilidades.cpp
  src/progressao/Inventario.cpp
  src/progressao/Equipamento.cpp
  src/progressao/Proficiencia.cpp
  src/missoes/QuestManager.cpp
  src/dialogo/DialogoDados.cpp
  src/save/SaveSystem.cpp
  src/combate/Armas.cpp
  src/combate/Recursos.cpp
)
target_include_directories(andv_nucleo PUBLIC include)
target_link_libraries(andv_nucleo PUBLIC nlohmann_json::nlohmann_json)
andv_aplicar_alvo(andv_nucleo)

# -----------------------------------------------------------------------------
#  Executável do jogo
# -----------------------------------------------------------------------------
add_executable(andvarion
  src/main.cpp
  src/core/Jogo.cpp
  src/core/Assets.cpp
  src/core/Entrada.cpp
  src/core/Roteiro.cpp
  src/gfx/SpriteSheet.cpp
  src/gfx/AnimationController.cpp
  src/gfx/PaperDoll.cpp
  src/gfx/VFXManager.cpp
  src/gfx/Iluminacao.cpp
  src/gfx/CameraJogo.cpp
  src/gfx/FilaRender.cpp
  src/mundo/MapaTiles.cpp
  src/entidades/Entidade.cpp
  src/entidades/Jogador.cpp
  src/entidades/Inimigo.cpp
  src/entidades/NPC.cpp
  src/entidades/Projetil.cpp
  src/entidades/Coletavel.cpp
  src/entidades/AreaDano.cpp
  src/entidades/Familiar.cpp
  src/combate/PlayerStateMachine.cpp
  src/combate/EstadosJogador.cpp
  src/dialogo/DialogueSystem.cpp
  src/ui/Fonte.cpp
  src/ui/Pergaminho.cpp
  src/ui/HUD.cpp
  src/ui/MenuArvoreHabilidades.cpp
  src/ui/MenuInventario.cpp
  src/ui/Notificacoes.cpp
  src/cenas/CenaMenuPrincipal.cpp
  src/cenas/CenaJogo.cpp
  src/cenas/Mundo.cpp
  src/cenas/Sessao.cpp
)
target_link_libraries(andvarion PRIVATE andv_nucleo raylib)
andv_aplicar_alvo(andvarion)

if(WIN32)
  # Sem console no Release; mantém o console no Debug para logs.
  if(MSVC)
    target_link_options(andvarion PRIVATE
      $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS> $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
  else()
    target_link_options(andvarion PRIVATE $<$<CONFIG:Release>:-mwindows>)
  endif()
  target_link_libraries(andvarion PRIVATE winmm)
endif()

# Copia assets/ para ao lado do executável a cada build.
add_custom_command(TARGET andvarion POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:andvarion>/assets"
  COMMENT "Copiando assets/")

# -----------------------------------------------------------------------------
#  Pipelines de arte
#    pixelfx            -> assets/vfx        (node tools/gerar_vfx.mjs)
#    agent-sprite-forge -> assets/sprites    (export_engine.py --target aseprite-json)
# -----------------------------------------------------------------------------
find_program(NODE_EXE node)
if(NODE_EXE)
  add_custom_target(vfx
    COMMAND "${NODE_EXE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/gerar_vfx.mjs"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "pixelfx: gerando sprite sheets de VFX em assets/vfx"
    VERBATIM)
  if(ANDV_GERAR_VFX)
    add_dependencies(andvarion vfx)
  endif()
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
  add_custom_target(placeholders
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/gerar_placeholders.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Gerando sprites/tileset/retratos provisórios no formato agent-sprite-forge"
    VERBATIM)
endif()

# -----------------------------------------------------------------------------
#  Testes
# -----------------------------------------------------------------------------
if(ANDV_TESTES)
  enable_testing()
  add_executable(andv_testes tests/testes.cpp)
  target_link_libraries(andv_testes PRIVATE andv_nucleo)
  target_compile_definitions(andv_testes PRIVATE ANDV_DIR_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets")
  andv_aplicar_alvo(andv_testes)
  add_test(NAME logica COMMAND andv_testes)
endif()
