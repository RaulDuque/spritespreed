#include "andvarion/ui/MenuInventario.hpp"
#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/core/Jogo.hpp"
#include "andvarion/core/Texto.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/ui/Fonte.hpp"
#include "andvarion/ui/Pergaminho.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace andv {

namespace {
constexpr Rectangle PAINEL{24, 20, 592, 320};
constexpr CategoriaItem ABAS[4] = {CategoriaItem::Equipamento, CategoriaItem::Consumivel, CategoriaItem::Material,
                                   CategoriaItem::Missao};
constexpr SlotEquipamento SLOTS[5] = {SlotEquipamento::Arma, SlotEquipamento::Distancia, SlotEquipamento::Armadura,
                                      SlotEquipamento::Capa, SlotEquipamento::Escudo};
constexpr float LISTA_X = PAINEL.x + 12, LISTA_Y = PAINEL.y + 48, LISTA_L = 250, LINHA = 14;

// Nomes legíveis dos atributos para a descrição do item.
std::string textoAtributos(const DefinicaoItem& d) {
    static const std::pair<const char*, const char*> nomes[] = {
        {"defesa", "Defesa"}, {"vidaMax", "Vida"}, {"manaMax", "Mana"}, {"vigorMax", "Vigor"},
        {"danoMagia", "Magia"}, {"velocidade", "Velocidade"}};
    std::string r;
    for (const auto& [chave, nome] : nomes) {
        auto it = d.atributos.find(chave);
        if (it == d.atributos.end()) continue;
        char buf[48];
        const bool pct = std::fabs(it->second) < 1.f;
        if (pct) std::snprintf(buf, sizeof buf, "%s %+d%%  ", nome, static_cast<int>(std::lround(it->second * 100)));
        else std::snprintf(buf, sizeof buf, "%s %+d  ", nome, static_cast<int>(it->second));
        r += buf;
    }
    return r;
}
} // namespace

Rectangle MenuInventario::retanguloAba(int i) const { return {LISTA_X + i * 64.f, PAINEL.y + 28, 62, 14}; }
Rectangle MenuInventario::retanguloItem(int i) const { return {LISTA_X, LISTA_Y + i * LINHA, LISTA_L, LINHA - 1}; }

std::vector<std::string> MenuInventario::itensDaAba(const Sessao& s) const {
    std::vector<std::string> r;
    for (const auto& [id, d] : s.inventario.catalogo()) {
        if (d.categoria != ABAS[aba_]) continue;
        // Frasco de Sangue é um contador da sessão, não um item empilhado.
        const bool tem = id == "frasco_sangue" ? s.frascos > 0 : s.inventario.quantidade(id) > 0;
        if (tem) r.push_back(id);
    }
    // Equipamentos: na ordem dos slots, depois por nome.
    std::stable_sort(r.begin(), r.end(), [&](const std::string& a, const std::string& b) {
        const auto* da = s.inventario.definicao(a);
        const auto* db = s.inventario.definicao(b);
        return da->slot < db->slot;
    });
    return r;
}

void MenuInventario::acionar(const std::string& id, Sessao& s) {
    const DefinicaoItem* d = s.inventario.definicao(id);
    if (!d || d->categoria != CategoriaItem::Equipamento) {
        mensagem_ = d && d->categoria == CategoriaItem::Consumivel ? "Use os frascos com F durante o jogo." : "Este item não pode ser equipado.";
        tempoMensagem_ = 2.f;
        return;
    }
    const auto slot = slotDeChave(d->slot);
    if (slot && s.equipamento.equipado(id)) {
        if (s.equipamento.desequipar(*slot)) mensagem_ = "Removido: " + d->nome;
        else mensagem_ = "Sempre há uma arma em mãos: escolha outra para trocar.";
    } else if (s.equipamento.equipar(id, s.inventario) == ResultadoEquipar::Ok) {
        mensagem_ = "Equipado: " + d->nome;
    }
    tempoMensagem_ = 2.f;
    if (aoMudar_) aoMudar_();
}

void MenuInventario::atualizar(float dt, const ComandosJogador& cmd, Sessao& s) {
    if (!aberto_) return;
    tempo_ += dt;
    tempoMensagem_ = std::max(0.f, tempoMensagem_ - dt);
    if (cmd.inventario || cmd.cancelar || cmd.pausar) {
        fechar();
        return;
    }
    if (cmd.esquerda) { aba_ = (aba_ + 3) % 4; selecionado_ = 0; }
    if (cmd.direita) { aba_ = (aba_ + 1) % 4; selecionado_ = 0; }
    const auto itens = itensDaAba(s);
    const int n = static_cast<int>(itens.size());
    if (n > 0) {
        if (cmd.cima) selecionado_ = (selecionado_ + n - 1) % n;
        if (cmd.baixo) selecionado_ = (selecionado_ + 1) % n;
    }
    selecionado_ = std::clamp(selecionado_, 0, std::max(0, n - 1));
    for (int i = 0; i < 4; ++i)
        if (cmd.clique && CheckCollisionPointRec(cmd.mouseVirtual, retanguloAba(i))) { aba_ = i; selecionado_ = 0; return; }
    for (int i = 0; i < n; ++i) {
        if (!CheckCollisionPointRec(cmd.mouseVirtual, retanguloItem(i))) continue;
        selecionado_ = i;
        if (cmd.clique) { acionar(itens[static_cast<std::size_t>(i)], s); return; }
    }
    if (n > 0 && (cmd.confirmar || cmd.interagir)) acionar(itens[static_cast<std::size_t>(selecionado_)], s);
}

void MenuInventario::desenhar(const Fonte& fonte, const Sessao& s, const Jogador& j) const {
    if (!aberto_) return;
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, Color{0, 0, 0, 160});
    ui::desenharPergaminho(PAINEL);
    fonte.desenharCentro("Inventário", {PAINEL.x + PAINEL.width * 0.5f, PAINEL.y + 12}, 10.f, ui::DOURADO);

    // Abas.
    for (int i = 0; i < 4; ++i) {
        const Rectangle r = retanguloAba(i);
        if (i == aba_) DrawRectangleRec(r, Color{90, 30, 36, 220});
        DrawRectangleLinesEx(r, 1.f, i == aba_ ? ui::DOURADO : ui::PERGAMINHO_BORDA_ESCURA);
        const char* nome = i == 0 ? "Equip." : i == 1 ? "Consum." : i == 2 ? "Materiais" : "Missão";
        fonte.desenharCentro(nome, {r.x + r.width * 0.5f, r.y + 7}, 10.f, i == aba_ ? ui::DOURADO : ui::CINZA_TEXTO);
    }

    // Lista.
    const auto itens = itensDaAba(s);
    if (itens.empty()) fonte.desenhar("(vazio)", {LISTA_X + 4, LISTA_Y + 2}, 10.f, ui::CINZA_TEXTO);
    for (int i = 0; i < static_cast<int>(itens.size()); ++i) {
        const std::string& id = itens[static_cast<std::size_t>(i)];
        const DefinicaoItem* d = s.inventario.definicao(id);
        const Rectangle r = retanguloItem(i);
        const bool sel = i == selecionado_;
        const bool eq = s.equipamento.equipado(id);
        if (sel) DrawRectangleRec(r, Color{90, 30, 36, 200});
        std::string texto = (eq ? "[E] " : "    ") + d->nome;
        const i32 qtd = id == "frasco_sangue" ? s.frascos : s.inventario.quantidade(id);
        if (qtd > 1) texto += " x" + std::to_string(qtd);
        fonte.desenhar(texto, {r.x + 3, r.y + 2}, 10.f, eq ? ui::DOURADO : (sel ? ui::OSSO : ui::CINZA_TEXTO));
        if (d->categoria == CategoriaItem::Equipamento) {
            const std::string slot = nomeSlot(*slotDeChave(d->slot));
            fonte.desenhar(slot, {r.x + r.width - fonte.largura(slot) - 3, r.y + 2}, 10.f, Color{120, 110, 100, 255});
        }
    }

    // Prévia do personagem (paper-doll em resolução nativa da arte: 2x).
    const Rectangle previa{PAINEL.x + 290, PAINEL.y + 48, 130, 150};
    DrawRectangleRec(previa, Color{16, 12, 18, 230});
    DrawRectangleLinesEx(previa, 1.f, ui::PERGAMINHO_BORDA);
    const Vector2 pes{previa.x + previa.width * 0.5f, previa.y + previa.height - 14};
    DrawEllipse(static_cast<int>(pes.x), static_cast<int>(pes.y), 18, 5, Color{0, 0, 0, 120});
    if (j.anim.temSprites()) j.visual().desenhar(j.anim, pes, WHITE, 2.f);   // 2x: cada px de arte = 2 px

    // Slots e atributos.
    float y = PAINEL.y + 48;
    const float sx = PAINEL.x + 432;
    fonte.desenhar("Equipado", {sx, y}, 10.f, ui::DOURADO);
    y += 14;
    for (SlotEquipamento slot : SLOTS) {
        const std::string& id = s.equipamento.item(slot);
        fonte.desenhar(std::string(nomeSlot(slot)) + ":", {sx, y}, 10.f, ui::CINZA_TEXTO);
        fonte.desenhar(id.empty() ? "-" : s.inventario.nome(id), {sx + 8, y + 11}, 10.f, ui::OSSO);
        y += 25;
    }
    char buf[64];
    std::snprintf(buf, sizeof buf, "Defesa %d   Vida %d   Mana %d", static_cast<int>(j.defesa()), static_cast<int>(j.vidaMax),
                  static_cast<int>(j.recursos.mana.maximo));
    fonte.desenhar(buf, {previa.x, previa.y + previa.height + 6}, 10.f, ui::OSSO);

    // Descrição do item selecionado.
    const Rectangle info{PAINEL.x + 12, PAINEL.y + PAINEL.height - 66, PAINEL.width - 24, 46};
    DrawRectangleRec(info, Color{14, 9, 8, 200});
    if (!itens.empty()) {
        const DefinicaoItem* d = s.inventario.definicao(itens[static_cast<std::size_t>(selecionado_)]);
        fonte.desenhar(d->nome, {info.x + 5, info.y + 4}, 10.f, ui::DOURADO);
        fonte.desenhar(d->descricao, {info.x + 5, info.y + 17}, 10.f, ui::OSSO);
        fonte.desenhar(textoAtributos(*d), {info.x + 5, info.y + 30}, 10.f, Color{150, 220, 140, 255});
    }
    if (tempoMensagem_ > 0.f)
        fonte.desenhar(mensagem_, {info.x + info.width - fonte.largura(mensagem_) - 5, info.y + 4}, 10.f, ui::DOURADO);
    fonte.desenhar("Clique ou Enter: equipar/remover   A/D: abas   I: fechar", {PAINEL.x + 12, PAINEL.y + PAINEL.height - 16}, 10.f,
                   ui::CINZA_TEXTO);
}

} // namespace andv
