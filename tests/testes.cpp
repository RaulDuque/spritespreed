// Testes da lógica pura (sem janela, sem GPU). Rodam no CI/ctest em x86.
//   ctest --preset linux-i686           (ou: ./andv_testes [filtro])
#include "andvarion/combate/Armas.hpp"
#include "andvarion/combate/Recursos.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/core/Texto.hpp"
#include "andvarion/dialogo/DialogoDados.hpp"
#include "andvarion/gfx/AtlasAseprite.hpp"
#include "andvarion/missoes/QuestManager.hpp"
#include "andvarion/progressao/ArvoreHabilidades.hpp"
#include "andvarion/progressao/Equipamento.hpp"
#include "andvarion/progressao/Proficiencia.hpp"
#include "andvarion/progressao/Inventario.hpp"
#include "andvarion/progressao/Progressao.hpp"
#include "andvarion/save/SaveSystem.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#ifndef ANDV_DIR_ASSETS
#define ANDV_DIR_ASSETS "assets"
#endif

using namespace andv;

namespace {

int falhas = 0, verificacoes = 0;

#define VERIFICA(cond)                                                                    \
    do {                                                                                  \
        ++verificacoes;                                                                   \
        if (!(cond)) {                                                                    \
            ++falhas;                                                                     \
            std::printf("  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
        }                                                                                 \
    } while (0)

// Assets: o caminho do build ou, se não existir (executável copiado para
// outra máquina), a pasta assets/ ao lado / no diretório atual.
std::string raizAssets() {
    static const std::string raiz = [] {
        for (const char* r : {ANDV_DIR_ASSETS, "assets"})
            if (texto::lerArquivo(std::string(r) + "/dados/itens.json")) return std::string(r);
        return std::string(ANDV_DIR_ASSETS);
    }();
    return raiz;
}

std::string ler(const std::string& relativo) {
    auto t = texto::lerArquivo(raizAssets() + "/" + relativo);
    if (!t) std::printf("  arquivo ausente: %s\n", relativo.c_str());
    return t.value_or("");
}

// ---------------------------------------------------------------------------
void testePlataforma() {
#if !defined(ANDV_VERIFICACAO_HOST)
    VERIFICA(sizeof(void*) == 4);  // o alvo do projeto é x86
#endif
    VERIFICA(sizeof(i32) == 4);
}

void testeTexto() {
    const std::string s = "Ação é ótima";
    VERIFICA(texto::contarCodepoints(s) == 12);
    VERIFICA(texto::prefixo(s, 2) == "Aç");
    VERIFICA(texto::prefixo(s, 3) == "Açã");
    VERIFICA(texto::prefixo(s, 100) == s);
    auto largura = [](const std::string& t) { return static_cast<float>(texto::contarCodepoints(t)); };
    auto linhas = texto::quebrarLinhas("um dois três quatro", 9.f, largura);
    VERIFICA(linhas.size() == 3);
    VERIFICA(linhas[0] == "um dois");
}

void testeEventBus() {
    EventBus bus;
    int mortes = 0, todos = 0;
    {
        auto a = bus.inscrever(TipoEvento::InimigoMorto, [&](const Evento& e) { mortes += e.quantidade; });
        auto b = bus.inscreverTodos([&](const Evento&) { ++todos; });
        bus.publicar(TipoEvento::InimigoMorto, "cultista", 2);
        bus.publicar(TipoEvento::ItemColetado, "erva");
        VERIFICA(mortes == 2);
        VERIFICA(todos == 2);
    }
    // Inscrições destruídas não recebem mais nada (RAII).
    bus.publicar(TipoEvento::InimigoMorto, "cultista");
    VERIFICA(mortes == 2);

    // Publicar durante a entrega: vai para a fila, sem recursão.
    std::vector<std::string> ordem;
    auto c = bus.inscrever(TipoEvento::MissaoConcluida, [&](const Evento& e) {
        ordem.push_back("concluida:" + e.id);
        bus.publicar(TipoEvento::NivelSubiu, "", 2);
    });
    auto d = bus.inscrever(TipoEvento::NivelSubiu, [&](const Evento&) { ordem.push_back("nivel"); });
    bus.publicar(TipoEvento::MissaoConcluida, "x");
    VERIFICA(ordem.size() == 2 && ordem[0] == "concluida:x" && ordem[1] == "nivel");

    // Observador que cancela a própria inscrição durante a entrega.
    Inscricao auto_;
    int vezes = 0;
    auto_ = bus.inscrever(TipoEvento::AreaEntrada, [&](const Evento&) { ++vezes; auto_.cancelar(); });
    bus.publicar(TipoEvento::AreaEntrada, "vila");
    bus.publicar(TipoEvento::AreaEntrada, "vila");
    VERIFICA(vezes == 1);
}

void testeAtlasForge() {
    std::string erro;
    auto a = AtlasAseprite::deJson(ler("sprites/protagonista/protagonista.json"), &erro);
    VERIFICA(a.has_value());
    if (!a) {
        std::printf("  erro: %s\n", erro.c_str());
        return;
    }
    VERIFICA(a->larguraQuadro() == 48 && a->alturaQuadro() == 64);   // arte em resolução dobrada
    VERIFICA(std::fabs(a->ancoraX() - 24.f) < 0.01f);   // anchor_px exato do slice.data
    VERIFICA(std::fabs(a->ancoraY() - 62.f) < 0.01f);
    const ClipeAtlas* andar = a->clipe("andar_direita");
    VERIFICA(andar && andar->loop && andar->quadros.size() == 6);
    const ClipeAtlas* golpe = a->clipe("ataque1_baixo");
    VERIFICA(golpe && !golpe->loop);                     // repeat "1" => one-shot
    VERIFICA(golpe && golpe->eventos.size() == 1 && golpe->eventos[0].nome == "hit" && golpe->eventos[0].posicao == 2);
    VERIFICA(a->clipe("andar_esquerda") == nullptr);     // esquerda vem do espelhamento
    VERIFICA(golpe && golpe->posicaoEm(0.f) == 0 && golpe->posicaoEm(10000.f) == 3);

    // Formato "hash" do Aseprite + pingpong.
    const char* hash = R"({"frames":{"b 0":{"frame":{"x":0,"y":0,"w":8,"h":8},"duration":50},
        "b 1":{"frame":{"x":8,"y":0,"w":8,"h":8},"duration":50},"b 2":{"frame":{"x":16,"y":0,"w":8,"h":8},"duration":50}},
        "meta":{"image":"b.png","frameTags":[{"name":"vai_e_volta","from":0,"to":2,"direction":"pingpong"}]}})";
    auto b = AtlasAseprite::deJson(hash);
    VERIFICA(b && b->clipe("vai_e_volta") && b->clipe("vai_e_volta")->quadros == std::vector<i32>({0, 1, 2, 1}));
    VERIFICA(b && b->quadros()[1].x == 8);  // ordem do arquivo preservada
}

void testeAtlasPixelfx() {
    auto a = AtlasAseprite::deJson(ler("vfx/lamina_corte.json"));
    VERIFICA(a.has_value());
    if (!a) return;
    VERIFICA(a->pixelfx().has_value());
    VERIFICA(a->pixelfx() && a->pixelfx()->fps == 15.f);
    VERIFICA(a->clipes().size() == 1 && !a->clipes().begin()->second.loop);
    auto tocha = AtlasAseprite::deJson(ler("vfx/tocha.json"));
    VERIFICA(tocha && tocha->clipes().begin()->second.loop);  // meta.pixelfx.loop
    VERIFICA(tocha && tocha->ancoraY() > 0.f);
}

void testeProgressao() {
    Progressao p;
    VERIFICA(Progressao::xpParaProximo(1) == 40);
    VERIFICA(p.ganharXP(39) == 0 && p.nivel() == 1);
    VERIFICA(p.ganharXP(1) == 1 && p.nivel() == 2 && p.pontos() == 1 && p.xp() == 0);
    VERIFICA(p.ganharXP(1000) >= 3);
    VERIFICA(!p.gastarPontos(1000));
}

struct Mundinho {
    EventBus bus;
    Inventario inv;
    QuestManager q{bus, [this](const std::string& id) { return inv.quantidade(id); }};
    BancoDialogos dialogos;
    Mundinho() {
        std::string erro;
        VERIFICA(inv.carregarCatalogo(ler("dados/itens.json"), &erro));
        VERIFICA(q.carregar(ler("dados/missoes.json"), &erro));
        VERIFICA(dialogos.carregar(ler("dados/dialogos.json"), &erro));
        if (!erro.empty()) std::printf("  erro: %s\n", erro.c_str());
    }
    void coletar(const std::string& id, int n = 1) {
        for (int i = 0; i < n; ++i) {
            inv.adicionar(id);
            bus.publicar(TipoEvento::ItemColetado, id);
        }
    }
};

void testeMissoes() {
    Mundinho m;
    int prontas = 0, concluidas = 0;
    auto a = m.bus.inscrever(TipoEvento::MissaoProntaParaEntregar, [&](const Evento&) { ++prontas; });
    auto b = m.bus.inscrever(TipoEvento::MissaoConcluida, [&](const Evento&) { ++concluidas; });

    VERIFICA(m.q.todas().size() == 3);
    VERIFICA(m.q.estado("o_sussurro") == EstadoMissao::NaoIniciada);

    // Eventos antes de aceitar não contam.
    m.bus.publicar(TipoEvento::InimigoMorto, "cultista");
    VERIFICA(m.q.aceitar("o_sussurro"));
    VERIFICA(!m.q.aceitar("o_sussurro"));
    VERIFICA(m.q.missao("o_sussurro")->objetivos[2].atual == 0);

    m.bus.publicar(TipoEvento::NpcFalado, "curandeira");
    m.coletar("selo_profano");
    for (int i = 0; i < 4; ++i) m.bus.publicar(TipoEvento::InimigoMorto, "cultista");
    m.bus.publicar(TipoEvento::InimigoMorto, "carnical");  // outro alvo: ignorado
    VERIFICA(m.q.estado("o_sussurro") == EstadoMissao::Ativa);
    m.bus.publicar(TipoEvento::InimigoMorto, "cultista");
    VERIFICA(m.q.estado("o_sussurro") == EstadoMissao::ProntaParaEntregar);
    VERIFICA(prontas == 1);
    VERIFICA(m.q.rastreadas().size() == 1 && m.q.rastreadas()[0]->principal);
    const Recompensa* r = m.q.entregar("o_sussurro");
    VERIFICA(r && r->xp == 150);
    VERIFICA(concluidas == 1 && m.q.estado("o_sussurro") == EstadoMissao::Concluida);
    VERIFICA(m.q.entregar("o_sussurro") == nullptr);

    // Itens já no inventário contam ao aceitar.
    m.coletar("erva_palida", 4);
    VERIFICA(m.q.aceitar("cinzas_da_memoria"));
    VERIFICA(m.q.estado("cinzas_da_memoria") == EstadoMissao::ProntaParaEntregar);
    // Perder um item volta para ATIVA.
    m.inv.remover("erva_palida");
    m.q.sincronizarItens();
    VERIFICA(m.q.estado("cinzas_da_memoria") == EstadoMissao::Ativa);

    // Save/Load das missões.
    const auto inst = m.q.salvar();
    Mundinho outro;
    outro.inv.restaurar(m.inv.itens());
    outro.q.restaurar(inst);
    VERIFICA(outro.q.estado("o_sussurro") == EstadoMissao::Concluida);
    VERIFICA(outro.q.missao("cinzas_da_memoria")->objetivos[0].atual == 3);
}

void testeDialogos() {
    Mundinho m;
    const DialogoNpc* vigia = m.dialogos.npc("vigia");
    VERIFICA(vigia && vigia->nome == "Aldric, o Vigia");
    VERIFICA(vigia && escolherEntrada(*vigia, m.q, m.inv) == std::optional<std::string>("abertura"));
    m.q.aceitar("o_sussurro");
    VERIFICA(vigia && escolherEntrada(*vigia, m.q, m.inv) == std::optional<std::string>("lembrete"));

    const DialogoNpc* cura = m.dialogos.npc("curandeira");
    VERIFICA(cura && escolherEntrada(*cura, m.q, m.inv) == std::optional<std::string>("sussurros"));
    m.bus.publicar(TipoEvento::NpcFalado, "curandeira");
    VERIFICA(cura && escolherEntrada(*cura, m.q, m.inv) == std::optional<std::string>("oferta"));

    VERIFICA(avaliarCondicao("!missao:ferro_amaldicoado:ATIVA", m.q, m.inv));
    VERIFICA(!avaliarCondicao("item:erva_palida:1", m.q, m.inv));
    VERIFICA(avaliarCondicao("objetivo:o_sussurro:0:completo", m.q, m.inv));

    // Todo NPC tem um nó padrão alcançável em qualquer estado.
    for (const auto& [id, npc] : m.dialogos.npcs()) VERIFICA(escolherEntrada(npc, m.q, m.inv).has_value());

    // Referência quebrada é recusada no carregamento.
    BancoDialogos ruim;
    VERIFICA(!ruim.carregar(R"({"npcs":{"x":{"entradas":[{"no":"a"}],"nos":{"a":{"texto":"oi","proximo":"b"}}}}})"));
}

void testeArvore() {
    ArvoreHabilidades a;
    std::string erro;
    VERIFICA(a.carregar(ler("dados/arvore_habilidades.json"), &erro));
    Progressao p;
    VERIFICA(a.feiticoLiberado("orbe_sangue") && a.feiticoLiberado("nevoa_cortante"));  // custo 0
    VERIFICA(!a.feiticoLiberado("sangria_vital"));
    VERIFICA(a.desbloquear("fio_afiado", p) == ResultadoDesbloqueio::SemPontos);
    p.ganharXP(40);
    VERIFICA(a.desbloquear("golpe_brutal", p) == ResultadoDesbloqueio::RequisitoFaltando);
    VERIFICA(a.desbloquear("fio_afiado", p) == ResultadoDesbloqueio::Ok);
    VERIFICA(std::fabs(a.bonus("danoLamina") - 0.15f) < 1e-4f);
    VERIFICA(a.desbloquear("fio_afiado", p) == ResultadoDesbloqueio::JaDesbloqueado);
    // Todo requisito aponta para um nó existente; todo feitiço da tabela existe na árvore.
    for (const auto& n : a.nos())
        for (const auto& r : n.requisitos) VERIFICA(a.no(r) != nullptr);
    for (const auto& f : feiticos()) {
        bool achou = false;
        for (const auto& n : a.nos()) achou |= (n.efeito == TipoEfeitoHabilidade::Feitico && n.alvo == f.id);
        VERIFICA(achou);
    }
}

// Árvore literal: cada magia tem o próprio galho, os nós exigem o anterior do
// mesmo galho e os ramos são Guerreiro, Arqueiro e Mago.
void testeGalhos() {
    ArvoreHabilidades a;
    std::string erro;
    VERIFICA(a.carregar(ler("dados/arvore_habilidades.json"), &erro));
    VERIFICA(a.galhos().size() == 15);
    for (const auto& f : feiticos()) {
        const GalhoHabilidade* g = a.galho(f.id);
        VERIFICA(g != nullptr);                                   // um galho por magia
        const NoHabilidade* base = a.no(f.id);
        VERIFICA(base && base->galho == f.id && base->ordem == 0 && base->efeito == TipoEfeitoHabilidade::Feitico);
    }
    for (const auto& g : a.galhos()) VERIFICA(g.ramo == "guerreiro" || g.ramo == "arqueiro" || g.ramo == "mago");
    // Requisito implícito: "inferno" (ordem 2) exige "brasa_viva" (ordem 1).
    const NoHabilidade* inferno = a.no("inferno");
    VERIFICA(inferno && inferno->requisitos.size() == 1 && inferno->requisitos[0] == "brasa_viva");
    Progressao p;
    p.ganharXP(5000);
    VERIFICA(a.desbloquear("inferno", p) == ResultadoDesbloqueio::RequisitoFaltando);
    VERIFICA(a.desbloquear("bola_fogo", p) == ResultadoDesbloqueio::Ok);
    VERIFICA(a.desbloquear("brasa_viva", p) == ResultadoDesbloqueio::Ok);
    VERIFICA(std::fabs(a.bonus("bola_fogo.dano") - 0.3f) < 1e-4f);
    VERIFICA(a.feiticoLiberado("bola_fogo"));
    ArvoreHabilidades ruim;
    VERIFICA(!ruim.carregar(R"({"galhos":[],"nos":[{"id":"x","galho":"inexistente","efeito":{"tipo":"atributo"}}]})"));
}

void testeProficiencia() {
    Proficiencia p;
    VERIFICA(p.nivel("espada") == 1 && p.bonusDano("espada") == 0.f);
    VERIFICA(p.ganhar("espada", 14) == 0);
    VERIFICA(p.ganhar("espada", 1) == 2);                        // 15 XP no nível 1
    VERIFICA(std::fabs(p.bonusDano("espada") - 0.04f) < 1e-5f);
    VERIFICA(p.ganhar("fogo", 100000) == Proficiencia::NIVEL_MAXIMO);
    VERIFICA(p.ganhar("fogo", 10) == 0 && p.fracao("fogo") == 1.f);
    VERIFICA(std::fabs(p.reducaoCusto("fogo") - 0.18f) < 1e-5f);
    Proficiencia q;
    q.restaurar(p.todas());
    VERIFICA(q.nivel("espada") == 2 && q.nivel("fogo") == 10);
}

void testeSede() {
    SedeDeSangue s;
    VERIFICA(s.consumir() == 1.f);
    s.ativar(5.f, 1);
    VERIFICA(s.ativa() && s.multiplicadorVelocidade() > 1.f);
    VERIFICA(s.consumir() > 1.f);
    VERIFICA(!s.ativa());
    s.ativar(1.f, 2);
    s.atualizar(1.5f);
    VERIFICA(!s.ativa());
}

void testeArmas() {
    VERIFICA(armasCorpoACorpo().size() == 5);
    VERIFICA(armasDistancia().size() == 3);
    VERIFICA(armaCorpoPorId("foice") && armaCorpoPorId("foice")->arcoGraus > armaCorpoPorId("espada")->arcoGraus);
    VERIFICA(armaDistanciaPorId("bumerangue")->projetil == TipoProjetil::Bumerangue);
    VERIFICA(feiticos().size() == 12);
    for (const auto& f : feiticos()) VERIFICA(std::string(nomeEscola(f.escola)) != "?");
}

// Paper-doll: toda camada de equipamento tem os MESMOS clipes, com a mesma
// contagem de quadros e a mesma âncora do corpo base. Se isso quebrar, a
// armadura "descola" do corpo no meio de uma animação.
void testeCamadasPaperDoll() {
    auto corpo = AtlasAseprite::deJson(ler("sprites/protagonista/protagonista.json"));
    VERIFICA(corpo.has_value());
    if (!corpo) return;
    Inventario inv;
    VERIFICA(inv.carregarCatalogo(ler("dados/itens.json")));
    int camadas = 0;
    for (const auto& [id, d] : inv.catalogo()) {
        if (d.visual.empty()) continue;
        const std::string nome = d.visual.substr(d.visual.rfind('/') + 1);
        auto c = AtlasAseprite::deJson(ler(d.visual + "/" + nome + ".json"));
        VERIFICA(c.has_value());
        if (!c) continue;
        ++camadas;
        VERIFICA(c->larguraQuadro() == corpo->larguraQuadro() && c->alturaQuadro() == corpo->alturaQuadro());
        VERIFICA(c->ancoraX() == corpo->ancoraX() && c->ancoraY() == corpo->ancoraY());
        for (const auto& [clipe, cc] : corpo->clipes()) {
            const ClipeAtlas* outro = c->clipe(clipe);
            VERIFICA(outro && outro->quadros.size() == cc.quadros.size() && outro->duracaoTotalMs == cc.duracaoTotalMs);
        }
    }
    VERIFICA(camadas == 13);
}

void testeEquipamento() {
    Inventario inv;
    std::string erro;
    VERIFICA(inv.carregarCatalogo(ler("dados/itens.json"), &erro));
    Equipamento e;
    VERIFICA(e.equipar("armadura_cavaleiro", inv) == ResultadoEquipar::NaoPossui);
    VERIFICA(e.equipar("erva_palida", inv) == ResultadoEquipar::NaoEquipavel);
    inv.adicionar("armadura_cavaleiro");
    inv.adicionar("escudo_leao");
    inv.adicionar("espada_prata");
    inv.adicionar("machado_carrasco");
    VERIFICA(e.equipar("armadura_cavaleiro", inv) == ResultadoEquipar::Ok);
    VERIFICA(e.equipar("escudo_leao", inv) == ResultadoEquipar::Ok);
    VERIFICA(e.equipar("espada_prata", inv) == ResultadoEquipar::Ok);
    VERIFICA(e.item(SlotEquipamento::Armadura) == "armadura_cavaleiro");
    VERIFICA(std::fabs(e.bonus("defesa", inv) - 22.f) < 1e-4f);              // 14 + 8
    VERIFICA(e.bonus("velocidade", inv) < 0.f);                               // placas e escudo pesam
    VERIFICA(e.proximo(SlotEquipamento::Arma, inv, 1) == "machado_carrasco"); // só armas possuídas
    VERIFICA(!e.desequipar(SlotEquipamento::Arma));                           // sempre há arma
    VERIFICA(e.desequipar(SlotEquipamento::Escudo) && e.item(SlotEquipamento::Escudo).empty());
    // Perder o item tira do slot; save/load preserva os slots.
    inv.remover("armadura_cavaleiro");
    e.validar(inv);
    VERIFICA(e.item(SlotEquipamento::Armadura).empty());
    Equipamento outro;
    outro.restaurar(e.salvar(), inv);
    VERIFICA(outro.item(SlotEquipamento::Arma) == "espada_prata");
}

void testeSave() {
    SaveData d;
    d.area = "ruinas_catedral";
    d.posX = 120.5f;
    d.posY = 64.f;
    d.temPosicao = true;
    d.nivel = 4;
    d.habilidades = {"fio_afiado", "sede_voraz"};
    d.inventario = {{"erva_palida", 2}};
    d.equipamento = {{"armadura", "tunica_mago"}};
    d.proficiencias = {{"arco", {3, 7}}};
    d.grimorio = {"bola_fogo", "", "relampago", "teleporte"};
    d.missoes["o_sussurro"] = {EstadoMissao::Ativa, {1, 0, 3}};
    d.coletados = {"ruinas_erva_1"};
    auto volta = SaveSystem::deJson(SaveSystem::paraJson(d));
    VERIFICA(volta.has_value());
    if (!volta) return;
    VERIFICA(volta->area == "ruinas_catedral" && volta->temPosicao && volta->posX == 120.5f);
    VERIFICA(volta->nivel == 4 && volta->habilidades.size() == 2);
    VERIFICA(volta->inventario.at("erva_palida") == 2);
    VERIFICA(volta->equipamento.at("armadura") == "tunica_mago");
    VERIFICA(volta->proficiencias.at("arco") == std::make_pair(3, 7));
    VERIFICA(volta->grimorio.size() == 4 && volta->grimorio[3] == "teleporte");
    VERIFICA(volta->missoes.at("o_sussurro").estado == EstadoMissao::Ativa);
    VERIFICA(volta->missoes.at("o_sussurro").progresso == std::vector<i32>({1, 0, 3}));
    VERIFICA(!SaveSystem::deJson("{quebrado").has_value());
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::pair<const char*, std::function<void()>>> testes = {
        {"plataforma", testePlataforma}, {"texto", testeTexto},       {"eventbus", testeEventBus},
        {"atlas_forge", testeAtlasForge}, {"atlas_pixelfx", testeAtlasPixelfx}, {"progressao", testeProgressao},
        {"missoes", testeMissoes},       {"dialogos", testeDialogos}, {"arvore", testeArvore},
        {"sede", testeSede},             {"armas", testeArmas},       {"save", testeSave},
        {"equipamento", testeEquipamento}, {"paperdoll", testeCamadasPaperDoll},
        {"galhos", testeGalhos},           {"proficiencia", testeProficiencia},
    };
    for (const auto& [nome, f] : testes) {
        if (argc > 1 && std::strstr(nome, argv[1]) == nullptr) continue;
        const int antes = falhas;
        f();
        std::printf("[%s] %s\n", falhas == antes ? " ok " : "FALHA", nome);
    }
    std::printf("%d verificações, %d falhas (%d bits)\n", verificacoes, falhas, static_cast<int>(sizeof(void*) * 8));
    return falhas == 0 ? 0 : 1;
}
