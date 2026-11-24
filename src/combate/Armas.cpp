#include "andvarion/combate/Armas.hpp"

namespace andv {

const std::vector<ArmaCorpoACorpo>& armasCorpoACorpo() {
    // id, nome, alcance, arco, dano[3], duração[3], fração ativo, janela, empurrão, avanço, MP/acerto, vfx[3], escala
    static const std::vector<ArmaCorpoACorpo> tabela = {
        {"espada",  "Espada Longa de Prata", 26.f, 110.f, {10.f, 10.f, 16.f}, {0.28f, 0.28f, 0.42f}, 0.35f, 0.35f,  60.f, 45.f,  6.f,
         {"lamina_corte", "lamina_corte", "lamina_final"}, 0.30f},
        {"adaga",   "Adagas Gêmeas",         18.f,  80.f, { 6.f,  6.f,  9.f}, {0.16f, 0.16f, 0.24f}, 0.30f, 0.30f,  25.f, 60.f,  4.f,
         {"adaga_corte", "adaga_corte", "adaga_corte"}, 0.24f},
        {"machado", "Machado do Carrasco",   28.f, 140.f, {18.f, 18.f, 30.f}, {0.45f, 0.45f, 0.65f}, 0.45f, 0.40f, 110.f, 25.f,  9.f,
         {"machado_corte", "machado_corte", "machado_corte"}, 0.22f},
        {"foice",   "Foice do Ceifador",     34.f, 200.f, {12.f, 12.f, 20.f}, {0.38f, 0.38f, 0.55f}, 0.40f, 0.40f,  45.f, 20.f,  8.f,
         {"foice_corte", "foice_corte", "foice_corte"}, 0.24f},
        {"bastao",  "Bastão Arcano",         30.f,  90.f, { 8.f,  8.f, 14.f}, {0.30f, 0.30f, 0.45f}, 0.35f, 0.35f,  80.f, 30.f, 12.f,
         {"bastao_golpe", "bastao_golpe", "impacto_critico"}, 0.28f},
    };
    return tabela;
}

const std::vector<ArmaDistancia>& armasDistancia() {
    // id, nome, projétil, dano, velocidade, alcance, vigor, carga, mult, cadência, vfx, escala
    static const std::vector<ArmaDistancia> tabela = {
        {"arco",       "Arco de Teixo",       TipoProjetil::Flecha,     12.f, 260.f, 220.f, 12.f, 0.9f, 2.5f, 0.35f, "flecha",     0.22f},
        {"besta",      "Besta de Caçador",    TipoProjetil::Virote,     20.f, 340.f, 260.f, 18.f, 1.2f, 2.2f, 0.70f, "virote",     0.20f},
        {"bumerangue", "Bumerangue de Osso",  TipoProjetil::Bumerangue,  9.f, 200.f, 110.f, 10.f, 0.8f, 2.0f, 0.20f, "bumerangue", 0.18f},
    };
    return tabela;
}

const std::vector<DefinicaoFeitico>& feiticos() {
    // id, nome, escola, mana, recarga, vfx, descrição
    static const std::vector<DefinicaoFeitico> tabela = {
        {"orbe_sangue", "Orbe de Sangue", "sangue", 14.f, 0.7f, "orbe_sangue",
         "Uma esfera de sangue coagulado que explode ao tocar o inimigo."},
        {"nevoa_cortante", "Névoa Cortante", "sombra", 20.f, 4.0f, "nevoa_cortante",
         "Uma bruma de lâminas invisíveis que fere tudo dentro dela."},
        {"correntes_sombrias", "Correntes Sombrias", "sombra", 26.f, 8.0f, "correntes_sombrias",
         "Mãos de sombra brotam do chão e prendem os inimigos."},
        {"sangria_vital", "Sangria Vital", "sangue", 30.f, 10.0f, "sangria_vital",
         "Arranca o sangue dos inimigos próximos e o devolve como vida."},
        {"bola_fogo", "Bola de Fogo", "fogo", 18.f, 1.2f, "bola_fogo",
         "Uma esfera de chamas que explode em área ao atingir algo."},
        {"lanca_gelo", "Lança de Gelo", "gelo", 16.f, 1.0f, "lanca_gelo",
         "Atravessa a fila de inimigos e os deixa lentos."},
        {"relampago", "Relâmpago em Cadeia", "raio", 24.f, 3.0f, "relampago",
         "Um raio salta de inimigo em inimigo."},
        {"escudo_arcano", "Escudo Arcano", "arcano", 22.f, 12.0f, "escudo_arcano",
         "Uma esfera de força absorve o próximo dano recebido."},
        {"drenar_vida", "Drenar Vida", "sangue", 18.f, 2.5f, "drenar_vida",
         "Suga a vida do inimigo mais próximo da mira."},
        {"familiar", "Invocação de Familiar", "arcano", 30.f, 25.0f, "familiar",
         "Um espírito familiar orbita você e dispara contra os inimigos."},
        {"teleporte", "Teleporte Curto", "arcano", 12.f, 2.0f, "teleporte_entrada",
         "Some e reaparece na direção da mira, atravessando o perigo."},
        {"encantamento_lamina", "Encantamento de Lâmina", "fogo", 20.f, 15.0f, "encantamento",
         "Envolve a lâmina em fogo: golpes corpo a corpo causam dano extra."},
    };
    return tabela;
}

const DefinicaoFeitico* feiticoPorId(const std::string& id) {
    for (const auto& f : feiticos())
        if (f.id == id) return &f;
    return nullptr;
}

const char* nomeEscola(const std::string& e) {
    if (e == "sangue") return "Sangue";
    if (e == "fogo") return "Fogo";
    if (e == "gelo") return "Gelo";
    if (e == "raio") return "Raio";
    if (e == "arcano") return "Arcano";
    if (e == "sombra") return "Sombra";
    return "?";
}

const ArmaCorpoACorpo* armaCorpoPorId(const std::string& id) {
    for (const auto& a : armasCorpoACorpo())
        if (a.id == id) return &a;
    return nullptr;
}

const ArmaDistancia* armaDistanciaPorId(const std::string& id) {
    for (const auto& a : armasDistancia())
        if (a.id == id) return &a;
    return nullptr;
}

} // namespace andv
