#pragma once
// Tipos de largura fixa e a trava de arquitetura 32 bits.
//
// Todos os dados que vão para disco (save, mapas) usam int32/float, nunca size_t
// ou long, para que um save gerado em qualquer build x86 seja idêntico.
#include <cstdint>

namespace andv {

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;

} // namespace andv

#if defined(ANDV_ALVO_32_BITS) && !defined(ANDV_VERIFICACAO_HOST)
static_assert(sizeof(void*) == 4, "Andvarion deve ser compilado para x86 (32 bits). Use um preset win32-msvc, mingw-i686 ou linux-i686.");
#endif
