#pragma once
// Armas — tabela de dados do arsenal medieval (sem lógica de render).
//
// Cada arquétipo corpo a corpo tem alcance, arco de acerto, velocidade e dano
// por golpe do combo de 3, além do VFX do pixelfx de cada golpe. Para equilibrar
// uma arma, mude só os números aqui.
#include "andvarion/core/Plataforma.hpp"

#include <array>
#include <string>
#include <vector>

namespace andv {

struct ArmaCorpoACorpo {
    std::string id;
    std::string nome;
    float alcance = 24.f;                 // px a partir do centro do jogador
    float arcoGraus = 110.f;              // abertura do golpe
    std::array<float, 3> dano{};          // golpes 1, 2 e 3 do combo
    std::array<float, 3> duracao{};       // segundos de cada golpe (preparação+ativo+recuperação)
    float fracaoAtivo = 0.35f;            // em que fração do golpe o acerto acontece
    float janelaCombo = 0.35f;            // tempo após o golpe para emendar o próximo
    float empurrao = 60.f;                // px/s aplicado ao alvo
    float avanco = 40.f;                  // px/s de passo à frente durante o golpe
    float manaPorAcerto = 6.f;            // essência vital drenada -> MP
    std::array<std::string, 3> vfx;       // chaves do VFXManager
    float escalaVfx = 0.3f;
};

enum class TipoProjetil : u8 { Flecha, Virote, Bumerangue, OrbeSangue, RaioSombrio, BolaFogo, LancaGelo, Familiar };

struct ArmaDistancia {
    std::string id;
    std::string nome;
    TipoProjetil projetil = TipoProjetil::Flecha;
    float dano = 10.f;
    float velocidade = 260.f;             // px/s
    float alcance = 220.f;                // px (bumerangue: distância antes de voltar)
    float custoVigor = 12.f;
    float tempoCarga = 0.9f;              // segurar o botão por isso = disparo carregado
    float multCarga = 2.5f;               // dano do disparo carregado
    float cadencia = 0.35f;               // intervalo mínimo entre disparos
    std::string vfx;
    float escalaVfx = 0.25f;
};

// Grimório: todas as magias do jogo. Cada uma é um galho da árvore de
// habilidades; as aprendidas são colocadas nos 4 slots (teclas 1, 2, 3 e Q).
struct DefinicaoFeitico {
    std::string id;
    std::string nome;
    std::string escola;          // "sangue", "fogo", "gelo", "raio", "arcano", "sombra" (proficiência)
    float custoMana = 10.f;
    float recarga = 1.f;
    std::string vfx;             // ícone e efeito principal (pixelfx)
    std::string descricao;
};

inline constexpr int SLOTS_GRIMORIO = 4;
inline constexpr const char* TECLAS_GRIMORIO[SLOTS_GRIMORIO] = {"1", "2", "3", "Q"};

const std::vector<DefinicaoFeitico>& feiticos();
const DefinicaoFeitico* feiticoPorId(const std::string& id);
const char* nomeEscola(const std::string& escola);   // "Sangue", "Fogo", ...

const std::vector<ArmaCorpoACorpo>& armasCorpoACorpo();
const std::vector<ArmaDistancia>& armasDistancia();
const ArmaCorpoACorpo* armaCorpoPorId(const std::string& id);
const ArmaDistancia* armaDistanciaPorId(const std::string& id);

} // namespace andv
