#pragma once
// Utilitários de texto UTF-8 e arquivos.
//
// Todo o jogo é em PT-BR, então "ç", "ã", "é" ocupam 2 bytes. O efeito de
// máquina de escrever e a quebra de linha contam CODEPOINTS, nunca bytes,
// senão um caractere acentuado seria cortado ao meio.
#include "andvarion/core/Plataforma.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace andv::texto {

// Número de codepoints de uma string UTF-8.
i32 contarCodepoints(const std::string& s);

// Quantos bytes ocupam os primeiros `n` codepoints.
std::size_t bytesDosPrimeiros(const std::string& s, i32 n);

// Os primeiros `n` codepoints.
inline std::string prefixo(const std::string& s, i32 n) { return s.substr(0, bytesDosPrimeiros(s, n)); }

// Quebra `s` em linhas cuja largura (medida por `largura`) não passa de `max`.
// Respeita '\n' explícitos. Palavras maiores que `max` ficam sozinhas na linha.
std::vector<std::string> quebrarLinhas(const std::string& s, float max,
                                       const std::function<float(const std::string&)>& largura);

// Lê um arquivo inteiro (binário). std::nullopt se não abrir.
std::optional<std::string> lerArquivo(const std::string& caminho);

// Grava atomicamente: escreve em <caminho>.tmp e renomeia.
bool gravarArquivo(const std::string& caminho, const std::string& conteudo);

} // namespace andv::texto
