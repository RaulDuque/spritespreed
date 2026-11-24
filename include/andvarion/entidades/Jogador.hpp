#pragma once
// Jogador — o jovem guerreiro mágico de cabelos prateados.
//
// Guarda recursos, arsenal, recargas e a máquina de estados; as AÇÕES (golpe,
// disparo, feitiço, esquiva) ficam aqui como métodos para os estados chamarem.
#include "andvarion/combate/Armas.hpp"
#include "andvarion/combate/PlayerStateMachine.hpp"
#include "andvarion/combate/Recursos.hpp"
#include "andvarion/entidades/Entidade.hpp"
#include "andvarion/gfx/PaperDoll.hpp"

#include <array>
#include <deque>
#include <map>
#include <string>
#include <vector>

namespace andv {

struct Sessao;

struct Afterimage {
    std::vector<QuadroCongelado> camadas;   // paper-doll congelado
    Vector2 pos;
    float vida, total;
};

class Jogador final : public Combatente {
public:
    Jogador();

    void iniciar(Mundo& m);                       // ao entrar numa área
    void atualizarJogador(Mundo& m, const ComandosJogador& cmd, float dt);
    void atualizar(Mundo& m, float dt) override;  // não usado (o Mundo chama atualizarJogador)
    void desenhar(const Mundo& m) const override;
    void desenharAfterimages() const;
    bool receberDano(Mundo& m, const Dano& bruto) override;

    // --- atributos derivados (base + árvore + equipamento + forja) ---
    void recalcularAtributos(const Sessao& s, bool encher);
    // Lê os slots da Sessao: arma/distância ativas e as camadas do paper-doll.
    void aplicarEquipamento(Mundo& m);
    float defesa() const;
    float velocidadeMovimento() const;
    float multiplicadorLamina() const;
    float multiplicadorDistancia() const;
    float multiplicadorMagia() const;

    // --- ações chamadas pelos estados ---
    void mover(Mundo& m, Vector2 direcao, float velocidade, float dt);
    void olharPara(Vector2 alvoMundo);
    void executarGolpe(Mundo& m, int indice, Vector2 dir, float multDano);
    bool disparar(Mundo& m, Vector2 alvo, float carga01, float multDano);
    bool podeConjurar(const Mundo& m, int slot, std::string* motivo) const;
    void lancarFeitico(Mundo& m, int slot, Vector2 alvo);
    const std::string& feiticoNoSlot(int slot) const;   // grimório da sessão
    float recarga(const std::string& feitico) const;     // segundos restantes
    float custoMana(const DefinicaoFeitico& f) const;    // com o desconto da proficiência
    void ganharProficiencia(Mundo& m, const std::string& chave, i32 xp);
    void iniciarEsquiva(Mundo& m);
    void criarAfterimage();
    bool usarFrasco(Mundo& m);

    const ArmaCorpoACorpo& armaCorpo() const { return *armaCorpo_; }
    const ArmaDistancia& armaDistancia() const { return *armaDistancia_; }
    void ciclarCorpo(Mundo& m, int passo);
    void ciclarDistancia(Mundo& m);
    PaperDoll& visual() { return visual_; }
    const PaperDoll& visual() const { return visual_; }

    Recursos recursos;
    SedeDeSangue sede;
    PlayerStateMachine estados;
    Vector2 mira{};                                // cursor no mundo
    // Efeitos ativos de magias
    float escudoPontos = 0.f, escudoTempo = 0.f;   // Escudo Arcano
    float encantamentoTempo = 0.f;                 // Encantamento de Lâmina
    float recargaDisparo = 0.f;
    float recargaEsquiva = 0.f;
    float janelaCombo = 0.f;                       // tempo para emendar o próximo golpe após voltar ao ocioso
    int   proximoCombo = 0;
    bool  bumerangueEmVoo = false;
    std::deque<Afterimage> afterimages;
    float tempoDesdeDano = 99.f;
    float cargaVisual = -1.f;                      // >= 0 enquanto mira (0..1)
    int   comboVisual = -1;                        // golpe atual (para o HUD/debug)
    // Feedback para a UI ("Mana insuficiente", "Sem vigor")
    std::string aviso;
    float avisoTempo = 0.f;
    void avisar(const std::string& texto) { aviso = texto; avisoTempo = 1.6f; }

private:
    const ArmaCorpoACorpo* armaCorpo_ = nullptr;
    const ArmaDistancia* armaDistancia_ = nullptr;
    const Sessao* sessao_ = nullptr;
    PaperDoll visual_;
    std::shared_ptr<const ConjuntoSprites> visualArma_, visualDistancia_;  // a mão mostra uma das duas
    bool mostrandoDistancia_ = false;
    unsigned luzLanterna_ = 0;
    unsigned vfxSede_ = 0;
    unsigned vfxEscudo_ = 0, vfxEncantamento_ = 0;
    std::map<std::string, float> recargas_;        // por magia: trocar de slot não zera a recarga
    float bonusFeitico(const std::string& feitico, const char* chave) const;
    Combatente* alvoMaisProximo(Mundo& m, Vector2 mira, float alcance);
};

} // namespace andv
