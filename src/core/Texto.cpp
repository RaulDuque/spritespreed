#include "andvarion/core/Texto.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace andv::texto {

namespace {
// Byte de continuação UTF-8: 10xxxxxx
bool continuacao(unsigned char c) { return (c & 0xC0) == 0x80; }
}

i32 contarCodepoints(const std::string& s) {
    i32 n = 0;
    for (unsigned char c : s)
        if (!continuacao(c)) ++n;
    return n;
}

std::size_t bytesDosPrimeiros(const std::string& s, i32 n) {
    if (n <= 0) return 0;
    i32 vistos = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (!continuacao(static_cast<unsigned char>(s[i]))) {
            if (vistos == n) return i;
            ++vistos;
        }
    }
    return s.size();
}

std::vector<std::string> quebrarLinhas(const std::string& s, float max,
                                       const std::function<float(const std::string&)>& largura) {
    std::vector<std::string> linhas;
    std::istringstream paragrafos(s);
    std::string paragrafo;
    while (std::getline(paragrafos, paragrafo, '\n')) {
        std::istringstream palavras(paragrafo);
        std::string palavra, linha;
        while (palavras >> palavra) {
            const std::string tentativa = linha.empty() ? palavra : linha + " " + palavra;
            if (!linha.empty() && largura(tentativa) > max) {
                linhas.push_back(linha);
                linha = palavra;
            } else {
                linha = tentativa;
            }
        }
        linhas.push_back(linha);  // parágrafo vazio vira linha vazia
    }
    if (linhas.empty()) linhas.emplace_back();
    return linhas;
}

std::optional<std::string> lerArquivo(const std::string& caminho) {
    std::ifstream f(caminho, std::ios::binary);
    if (!f) return std::nullopt;
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool gravarArquivo(const std::string& caminho, const std::string& conteudo) {
    const std::string tmp = caminho + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << conteudo;
        if (!f) return false;
    }
    std::remove(caminho.c_str());  // rename não sobrescreve no Windows
    return std::rename(tmp.c_str(), caminho.c_str()) == 0;
}

} // namespace andv::texto
