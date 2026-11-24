#include "andvarion/gfx/AnimationController.hpp"

#include <algorithm>
#include <cmath>

namespace andv {

namespace {

constexpr const char* NOMES_PT[] = {"baixo", "baixo_direita", "direita", "cima_direita",
                                    "cima", "cima_esquerda", "esquerda", "baixo_esquerda"};
constexpr const char* NOMES_EN[] = {"down", "down_right", "right", "up_right",
                                    "up", "up_left", "left", "down_left"};

Direcao espelho(Direcao d) {
    switch (d) {
        case Direcao::Direita:        return Direcao::Esquerda;
        case Direcao::Esquerda:       return Direcao::Direita;
        case Direcao::CimaDireita:    return Direcao::CimaEsquerda;
        case Direcao::CimaEsquerda:   return Direcao::CimaDireita;
        case Direcao::BaixoDireita:   return Direcao::BaixoEsquerda;
        case Direcao::BaixoEsquerda:  return Direcao::BaixoDireita;
        default:                      return d;
    }
}

// Diagonais caem para o lado horizontal (top-down costuma ter perfis laterais).
Direcao cardinal(Direcao d) {
    switch (d) {
        case Direcao::BaixoDireita: case Direcao::CimaDireita:   return Direcao::Direita;
        case Direcao::BaixoEsquerda: case Direcao::CimaEsquerda: return Direcao::Esquerda;
        default:                                                 return d;
    }
}

Direcao vertical(Direcao d) {
    switch (d) {
        case Direcao::BaixoDireita: case Direcao::BaixoEsquerda: return Direcao::Baixo;
        case Direcao::CimaDireita: case Direcao::CimaEsquerda:   return Direcao::Cima;
        default:                                                 return d;
    }
}

// Sinônimos de ação em PT-BR e nos nomes usuais da forge (inglês).
std::vector<std::string> sinonimos(const std::string& acao) {
    static const std::unordered_map<std::string, std::vector<std::string>> tabela = {
        {"ocioso",   {"ocioso", "idle"}},
        {"andar",    {"andar", "walk", "correr", "run"}},
        {"correr",   {"correr", "run", "andar", "walk"}},
        {"ataque1",  {"ataque1", "attack1", "ataque", "attack"}},
        {"ataque2",  {"ataque2", "attack2", "ataque1", "attack1", "ataque", "attack"}},
        {"ataque3",  {"ataque3", "attack3", "ataque1", "attack1", "ataque", "attack"}},
        {"disparo",  {"disparo", "shoot", "ranged", "ataque1", "attack"}},
        {"mirar",    {"mirar", "aim", "disparo", "shoot"}},
        {"conjurar", {"conjurar", "cast", "ataque1", "attack"}},
        {"esquiva",  {"esquiva", "dash", "roll", "correr", "run"}},
        {"dano",     {"dano", "hurt", "hit"}},
        {"morte",    {"morte", "death", "die", "dano", "hurt"}},
    };
    auto it = tabela.find(acao);
    return it != tabela.end() ? it->second : std::vector<std::string>{acao};
}

} // namespace

Direcao direcaoDeVetor(Vector2 v, bool oito) {
    if (std::fabs(v.x) < 1e-4f && std::fabs(v.y) < 1e-4f) return Direcao::Baixo;
    const float ang = std::atan2(v.y, v.x);                       // y para baixo
    const float passo = oito ? (PI / 4.f) : (PI / 2.f);
    i32 setor = static_cast<i32>(std::lround(ang / passo));       // 0 = direita
    if (!oito) setor *= 2;
    setor = ((setor % 8) + 8) % 8;
    // setor 0=direita,1=baixo_dir,2=baixo,3=baixo_esq,4=esq,5=cima_esq,6=cima,7=cima_dir
    static const Direcao mapa[8] = {Direcao::Direita, Direcao::BaixoDireita, Direcao::Baixo, Direcao::BaixoEsquerda,
                                    Direcao::Esquerda, Direcao::CimaEsquerda, Direcao::Cima, Direcao::CimaDireita};
    return mapa[setor];
}

Vector2 vetorDeDirecao(Direcao d) {
    constexpr float k = 0.70710678f;
    switch (d) {
        case Direcao::Baixo:         return {0, 1};
        case Direcao::BaixoDireita:  return {k, k};
        case Direcao::Direita:       return {1, 0};
        case Direcao::CimaDireita:   return {k, -k};
        case Direcao::Cima:          return {0, -1};
        case Direcao::CimaEsquerda:  return {-k, -k};
        case Direcao::Esquerda:      return {-1, 0};
        case Direcao::BaixoEsquerda: return {-k, k};
    }
    return {0, 1};
}

const char* nomeDirecao(Direcao d) { return NOMES_PT[static_cast<i32>(d)]; }

void AnimationController::definirConjunto(std::shared_ptr<const ConjuntoSprites> conjunto) {
    conjunto_ = std::move(conjunto);
    cache_.clear();
    atual_ = {};
    acao_.clear();
}

AnimationController::Resolucao AnimationController::resolver(const std::string& acao, Direcao dir) {
    const std::string chave = acao + "#" + NOMES_PT[static_cast<i32>(dir)];
    auto it = cache_.find(chave);
    if (it != cache_.end()) return it->second;

    Resolucao r;
    if (conjunto_) {
        struct Candidata { Direcao d; bool espelhar; };
        const Candidata dirs[] = {
            {dir, false}, {cardinal(dir), false}, {espelho(dir), true},
            {espelho(cardinal(dir)), true}, {vertical(dir), false},
        };
        auto tentar = [&](const std::string& nome, bool espelhar) {
            const auto achado = conjunto_->buscar(nome);
            if (!achado.clipe) return false;
            r = {achado.sheet, achado.clipe, espelhar};
            return true;
        };
        bool ok = false;
        for (const auto& a : sinonimos(acao)) {
            for (const auto& c : dirs) {
                const i32 i = static_cast<i32>(c.d);
                if (tentar(a + "_" + NOMES_PT[i], c.espelhar) || tentar(a + "_" + NOMES_EN[i], c.espelhar)) {
                    ok = true;
                    break;
                }
            }
            if (ok || tentar(a, false)) {
                ok = true;
                break;
            }
        }
        if (!ok && acao != "ocioso") r = resolver("ocioso", dir);
    }
    cache_[chave] = r;
    return r;
}

void AnimationController::tocar(const std::string& acao, Direcao dir, bool reiniciar) {
    const Resolucao r = resolver(acao, dir);
    const bool mesmoClipe = r.clipe == atual_.clipe && r.espelhar == atual_.espelhar;
    const bool mesmaAcao = acao == acao_;
    acao_ = acao;
    direcao_ = dir;
    if (reiniciar || !mesmaAcao) tempoMs_ = 0.f;
    // Mesma ação, outra direção: preserva a fase (tempo) se o clipe for loop.
    else if (!mesmoClipe && r.clipe && !r.clipe->loop) tempoMs_ = 0.f;
    atual_ = r;
}

void AnimationController::coletarEventos(float antesMs, float depoisMs) {
    if (!atual_.clipe || atual_.clipe->eventos.empty()) return;
    const ClipeAtlas& c = *atual_.clipe;
    const float total = static_cast<float>(std::max(1, c.duracaoTotalMs));
    // Início de cada posição na linha do tempo.
    std::vector<float> inicio(c.duracoesMs.size(), 0.f);
    for (std::size_t i = 1; i < inicio.size(); ++i) inicio[i] = inicio[i - 1] + static_cast<float>(c.duracoesMs[i - 1]);

    // Intervalo (a, b]; no início do clipe (a == 0) o quadro 0 também conta.
    const float a = antesMs <= 0.f ? -0.001f : antesMs;
    const float b = depoisMs;
    if (b <= a) return;
    for (const auto& ev : c.eventos) {
        const float t = inicio[static_cast<std::size_t>(ev.posicao)];
        if (c.loop) {
            // Um evento dispara uma vez por volta cruzada.
            for (float k = std::floor((a - t) / total); t + k * total <= b; k += 1.f)
                if (t + k * total > a) eventos_.push_back(ev.nome);
        } else if (t > a && t <= b) {
            eventos_.push_back(ev.nome);
        }
    }
}

void AnimationController::atualizar(float dt, float velocidade) {
    eventos_.clear();
    if (congelado_ > 0.f) {
        congelado_ -= dt;
        return;
    }
    if (!atual_.clipe) return;
    const float antes = tempoMs_;
    tempoMs_ += dt * 1000.f * velocidade;
    if (!atual_.clipe->loop) tempoMs_ = std::min(tempoMs_, static_cast<float>(atual_.clipe->duracaoTotalMs));
    coletarEventos(antes, tempoMs_);
}

void AnimationController::avancarDistancia(float distanciaPx, float passadaPx) {
    eventos_.clear();
    if (!atual_.clipe || passadaPx <= 0.f || congelado_ > 0.f) return;
    const float antes = tempoMs_;
    tempoMs_ += std::fabs(distanciaPx) / passadaPx * static_cast<float>(atual_.clipe->duracaoTotalMs);
    coletarEventos(antes, tempoMs_);
}

bool AnimationController::terminou() const {
    return !atual_.clipe || (!atual_.clipe->loop && tempoMs_ >= static_cast<float>(atual_.clipe->duracaoTotalMs));
}

float AnimationController::progresso() const {
    if (!atual_.clipe || atual_.clipe->duracaoTotalMs <= 0) return 1.f;
    const float total = static_cast<float>(atual_.clipe->duracaoTotalMs);
    return atual_.clipe->loop ? std::fmod(tempoMs_, total) / total : std::min(1.f, tempoMs_ / total);
}

bool AnimationController::disparou(const std::string& evento) const {
    return std::find(eventos_.begin(), eventos_.end(), evento) != eventos_.end();
}

QuadroCongelado AnimationController::quadroAtual() const {
    if (!atual_.clipe || !atual_.sheet || atual_.clipe->quadros.empty()) return {};
    const i32 pos = atual_.clipe->posicaoEm(tempoMs_);
    return {atual_.sheet, atual_.clipe->quadros[static_cast<std::size_t>(pos)], atual_.espelhar};
}

EstadoQuadro AnimationController::estado() const {
    if (!atual_.clipe || atual_.clipe->quadros.empty()) return {};
    return {atual_.clipe, atual_.clipe->posicaoEm(tempoMs_), atual_.espelhar};
}

void AnimationController::desenhar(Vector2 posPes, Color tinta, float escala) const {
    const QuadroCongelado q = quadroAtual();
    if (!q.sheet) return;
    // Snap do pivô no pixel de tela: evita tremido em pixel art.
    q.sheet->desenhar(q.quadro, {snapTela(posPes.x), snapTela(posPes.y)}, escala, q.espelhar, 0.f, tinta);
}

} // namespace andv
