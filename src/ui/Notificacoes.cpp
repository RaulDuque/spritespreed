#include "andvarion/ui/Notificacoes.hpp"
#include "andvarion/core/Jogo.hpp"
#include "andvarion/progressao/Proficiencia.hpp"
#include "andvarion/ui/Fonte.hpp"
#include "andvarion/ui/Pergaminho.hpp"

#include <algorithm>

namespace andv {

Notificacoes::Notificacoes(EventBus& bus, NomeMissao nome) {
    inscricoes_.push_back(bus.inscrever(TipoEvento::MissaoAceita, [this, nome](const Evento& e) {
        mostrar("Missão aceita: " + nome(e.id), ui::DOURADO, 4.f);
    }));
    inscricoes_.push_back(bus.inscrever(TipoEvento::MissaoProntaParaEntregar, [this, nome](const Evento& e) {
        mostrar("Objetivos cumpridos: " + nome(e.id), ui::OSSO, 4.f);
    }));
    inscricoes_.push_back(bus.inscrever(TipoEvento::MissaoConcluida, [this, nome](const Evento& e) {
        mostrar("Missão concluída: " + nome(e.id), Color{150, 220, 140, 255}, 4.5f);
    }));
    inscricoes_.push_back(bus.inscrever(TipoEvento::ProficienciaSubiu, [this](const Evento& e) {
        mostrar(std::string(nomeProficiencia(e.id)) + ": proficiência " + std::to_string(e.quantidade), Color{150, 200, 255, 255}, 3.5f);
    }));
    inscricoes_.push_back(bus.inscrever(TipoEvento::NivelSubiu, [this](const Evento& e) {
        mostrar("Nível " + std::to_string(e.quantidade) + "! +1 ponto de habilidade (K)", ui::DOURADO, 4.5f);
    }));
}

void Notificacoes::mostrar(const std::string& texto, Color cor, float duracao) {
    avisos_.push_back({texto, cor, duracao, duracao});
    while (avisos_.size() > 5) avisos_.pop_front();
}

void Notificacoes::atualizar(float dt) {
    for (auto& a : avisos_) a.restante -= dt;
    while (!avisos_.empty() && avisos_.front().restante <= 0.f) avisos_.pop_front();
}

void Notificacoes::desenhar(const Fonte& fonte) const {
    float y = ALTURA_VIRTUAL - 84.f;   // acima do diálogo e da barra de feitiços
    for (auto it = avisos_.rbegin(); it != avisos_.rend(); ++it) {
        const float alfa = std::clamp(std::min(it->restante, it->total - it->restante) * 4.f, 0.f, 1.f);
        Color c = it->cor;
        c.a = static_cast<unsigned char>(255 * alfa);
        const float w = fonte.largura(it->texto) + 12.f;
        DrawRectangle(static_cast<int>(LARGURA_VIRTUAL * 0.5f - w * 0.5f), static_cast<int>(y) - 2, static_cast<int>(w), 13,
                      Color{10, 6, 8, static_cast<unsigned char>(150 * alfa)});
        fonte.desenharCentro(it->texto, {LARGURA_VIRTUAL * 0.5f, y + 5.f}, 10.f, c);
        y -= 15.f;
    }
}

} // namespace andv
