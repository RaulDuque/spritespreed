#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/core/Assets.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/core/Jogo.hpp"
#include "andvarion/entidades/Coletavel.hpp"
#include "andvarion/entidades/Inimigo.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/entidades/NPC.hpp"
#include "andvarion/gfx/CameraJogo.hpp"
#include "andvarion/gfx/Iluminacao.hpp"
#include "andvarion/gfx/VFXManager.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace andv {

namespace {
Direcao direcaoDeTexto(const std::string& s) {
    if (s == "cima") return Direcao::Cima;
    if (s == "esquerda") return Direcao::Esquerda;
    if (s == "direita") return Direcao::Direita;
    return Direcao::Baixo;
}
} // namespace

Mundo::Mundo(Jogo& jogo, Sessao& sessao, Jogador& jogador, SistemaIluminacao& luz, CameraJogo& camera)
    : jogo_(jogo), sessao_(sessao), jogador_(jogador), luz_(luz), camera_(camera) {}

Mundo::~Mundo() = default;

VFXManager& Mundo::vfx() { return jogo_.vfx(); }
const VFXManager& Mundo::vfx() const { return jogo_.vfx(); }
EventBus& Mundo::bus() { return jogo_.bus(); }
Assets& Mundo::assets() { return jogo_.assets(); }

float Mundo::aleatorio(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng_); }

bool Mundo::carregarArea(const std::string& area, const std::string& spawn, std::optional<Vector2> posicao) {
    MapaTiles novo;
    std::string erro;
    if (!novo.carregar(assets(), "mapas/" + area + ".json", &erro)) {
        TraceLog(LOG_ERROR, "ANDVARION: %s", erro.c_str());
        return false;
    }
    // Ponteiros para objetos do mapa antigo morrem aqui junto com ele.
    transicaoProxima_ = nullptr;
    mapa_ = std::move(novo);
    entidades_.clear();
    novas_.clear();
    vfx().pararTodos();
    luz_.limpar();
    luz_.definirAmbiente(mapa_.ambiente());
    congelamento_ = 0.f;
    tempoTransicao_ = 0.f;
    pedidoTransicao.reset();

    if (posicao) {
        jogador_.pos = *posicao;
    } else if (const ObjetoMapa* s = mapa_.spawn(spawn)) {
        jogador_.pos = s->pos;
    }
    jogador_.iniciar(*this);
    popularObjetos();

    camera_.definirLimites(mapa_.limitesPx());
    camera_.focarImediato(jogador_.pos);
    bus().publicar(TipoEvento::AreaEntrada, mapa_.id());
    return true;
}

void Mundo::popularObjetos() {
    for (const ObjetoMapa& o : mapa_.objetos()) {
        if (o.tipo == "tocha" || o.tipo == "braseiro") {
            const bool tocha = o.tipo == "tocha";
            ParametrosVfx p;
            p.escala = tocha ? 0.22f : 0.2f;
            p.camada = CamadaVfx::Chao;
            vfx().tocar(tocha ? "tocha" : "braseiro", o.pos, p);
            Luz l;
            l.pos = {o.pos.x, o.pos.y - (tocha ? 14.f : 8.f)};
            l.raio = tocha ? 80.f : 105.f;
            l.cor = {255, 136, 56, 255};
            l.intensidade = 0.9f;
            l.oscilacao = 0.55f;        // luz oscilante das tochas
            l.brilho = 0.32f;
            l.fase = o.pos.x * 0.13f + o.pos.y * 0.07f;
            luz_.adicionar(l);
        } else if (o.tipo == "luz") {
            Luz l;
            l.pos = o.pos;
            l.raio = o.raio > 0.f ? o.raio : 64.f;
            l.cor = o.cor;
            l.intensidade = 0.7f;
            l.oscilacao = 0.15f;
            l.brilho = 0.25f;
            l.fase = o.pos.y;
            luz_.adicionar(l);
        } else if (o.tipo == "npc") {
            auto n = std::make_unique<NPC>(o.id, o.pos, direcaoDeTexto(o.direcao));
            n->anim.definirConjunto(assets().personagem(o.sprite.empty() ? "sprites/" + o.id : o.sprite));
            entidades_.push_back(std::move(n));
        } else if (o.tipo == "inimigo") {
            const DefinicaoInimigo* def = definicaoInimigo(o.classe);
            if (!def) {
                TraceLog(LOG_WARNING, "ANDVARION: inimigo desconhecido '%s'", o.classe.c_str());
                continue;
            }
            auto i = std::make_unique<Inimigo>(*def, o.pos, o.patrulha);
            i->anim.definirConjunto(assets().personagem(def->sprite));
            entidades_.push_back(std::move(i));
        } else if (o.tipo == "item") {
            if (!o.uid.empty() && sessao_.coletados.count(o.uid)) continue;
            soltarItem(o.id, o.pos, o.uid);
        } else if (o.tipo == "vfx") {
            ParametrosVfx p;
            p.escala = o.escala;
            p.camada = CamadaVfx::Frente;
            vfx().tocar(o.chave, o.pos, p);
        }
    }
    for (auto& n : novas_) entidades_.push_back(std::move(n));
    novas_.clear();
}

void Mundo::adicionar(std::unique_ptr<Entidade> e) { novas_.push_back(std::move(e)); }

void Mundo::soltarItem(const std::string& itemId, Vector2 pos, const std::string& uid) {
    const auto* def = sessao_.inventario.definicao(itemId);
    adicionar(std::make_unique<Coletavel>(itemId, pos, uid, def ? def->vfx : std::string("coleta")));
}

void Mundo::recompensarXP(i32 xp) {
    const i32 niveis = sessao_.progressao.ganharXP(xp);
    if (niveis <= 0) return;
    jogador_.recalcularAtributos(sessao_, true);  // subir de nível restaura tudo
    ParametrosVfx p;
    p.escala = 0.4f;
    p.seguir = [this] { return jogador_.pos; };
    vfx().tocar("subir_nivel", jogador_.pos, p);
    luz_.flash(jogador_.pos, 110.f, Color{255, 210, 120, 255}, 1.2f);
    bus().publicar(TipoEvento::NivelSubiu, {}, sessao_.progressao.nivel());
}

void Mundo::hitStop(float s) { congelamento_ = std::max(congelamento_, s); }

std::vector<Combatente*> Mundo::alvosNoRaio(Vector2 centro, float raio, Faccao atacante) {
    std::vector<Combatente*> r;
    if (atacante == Faccao::Inimigo) {
        if (jogador_.vivo() && Vector2Distance(centro, jogador_.pos) < raio + jogador_.raio) r.push_back(&jogador_);
        return r;
    }
    for (auto& e : entidades_) {
        if (e->faccao != Faccao::Inimigo || e->remover) continue;
        auto* c = dynamic_cast<Combatente*>(e.get());  // projéteis inimigos também têm a facção
        if (c && c->vivo() && Vector2Distance(centro, c->pos) < raio + c->raio) r.push_back(c);
    }
    return r;
}

NPC* Mundo::npcProximo(Vector2 pos, float raio) {
    NPC* melhor = nullptr;
    float menor = raio;
    for (auto& e : entidades_) {
        auto* n = dynamic_cast<NPC*>(e.get());
        if (!n) continue;
        const float d = Vector2Distance(pos, n->pos);
        if (d < menor) {
            menor = d;
            melhor = n;
        }
    }
    return melhor;
}

void Mundo::atualizarApresentacao(float dt) {
    luz_.atualizar(dt);
    vfx().atualizar(dt);
    camera_.atualizar(dt);
}

void Mundo::atualizar(float dt, const ComandosJogador& cmd) {
    // Relógio de apresentação: sempre corre (tremor, luzes, VFX).
    atualizarApresentacao(dt);
    tempoTransicao_ += dt;

    // Hit-stop: congela a simulação, não a apresentação.
    if (congelamento_ > 0.f) {
        congelamento_ -= dt;
        return;
    }

    jogador_.atualizarJogador(*this, cmd, dt);
    for (std::size_t i = 0; i < entidades_.size(); ++i)
        if (!entidades_[i]->remover) entidades_[i]->atualizar(*this, dt);
    for (auto& n : novas_) entidades_.push_back(std::move(n));
    novas_.clear();
    entidades_.erase(std::remove_if(entidades_.begin(), entidades_.end(), [](const auto& e) { return e->remover; }),
                     entidades_.end());

    // Separação simples entre inimigos (não se empilham no mesmo pixel).
    for (std::size_t a = 0; a < entidades_.size(); ++a) {
        if (!dynamic_cast<Inimigo*>(entidades_[a].get())) continue;
        for (std::size_t b = a + 1; b < entidades_.size(); ++b) {
            if (!dynamic_cast<Inimigo*>(entidades_[b].get())) continue;
            Vector2 d = Vector2Subtract(entidades_[b]->pos, entidades_[a]->pos);
            const float dist = Vector2Length(d), minimo = entidades_[a]->raio + entidades_[b]->raio;
            if (dist > 0.01f && dist < minimo) {
                d = Vector2Scale(d, (minimo - dist) * 0.5f / dist);
                entidades_[a]->pos = mapa_.moverCirculo(entidades_[a]->pos, entidades_[a]->raio, Vector2Negate(d));
                entidades_[b]->pos = mapa_.moverCirculo(entidades_[b]->pos, entidades_[b]->raio, d);
            }
        }
    }

    // NPC em destaque para o "[E] Conversar".
    NPC* perto = jogador_.vivo() ? npcProximo(jogador_.pos, 24.f) : nullptr;
    for (auto& e : entidades_)
        if (auto* n = dynamic_cast<NPC*>(e.get())) n->destacado = (n == perto);

    // Transições de cena.
    transicaoProxima_ = nullptr;
    for (const ObjetoMapa& o : mapa_.objetos()) {
        if (o.tipo != "transicao") continue;
        if (CheckCollisionPointRec(jogador_.pos, o.area)) {
            if (tempoTransicao_ > 0.6f && jogador_.vivo() && !pedidoTransicao)
                pedidoTransicao = PedidoTransicao{o.destino, o.spawn, o.rotulo};
        } else if (Vector2Distance(jogador_.pos, o.pos) < 40.f) {
            transicaoProxima_ = &o;
        }
    }

    const Vector2 olhar = Vector2Scale(Vector2Normalize(Vector2Subtract(jogador_.mira, jogador_.pos)), 1.f);
    camera_.seguir(jogador_.pos, olhar, dt);
}

void Mundo::desenhar() const {
    const Rectangle v = camera_.visivel();
    mapa_.desenharChao(v);
    vfx().desenhar(CamadaVfx::Chao);

    // Y-sort: decoração alta, entidades e o jogador, pelo Y dos pés.
    mapa_.enfileirarDecoracao(fila_, v);
    const Rectangle margem{v.x - 48, v.y - 48, v.width + 96, v.height + 96};
    for (const auto& e : entidades_) {
        if (!CheckCollisionPointRec(e->pos, margem)) continue;
        const Entidade* ptr = e.get();
        fila_.adicionar(ptr->ySort(), [this, ptr] { ptr->desenhar(*this); });
    }
    fila_.adicionar(jogador_.ySort(), [this] {
        jogador_.desenharAfterimages();
        jogador_.desenhar(*this);
    });
    fila_.desenharTudo();
    vfx().desenhar(CamadaVfx::Frente);

    // Saídas da área: um brilho no chão e o nome do destino ao se aproximar.
    for (const ObjetoMapa& o : mapa_.objetos()) {
        if (o.tipo != "transicao") continue;
        const float pulso = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 3.f);
        DrawRectangleLinesEx(o.area, 1.f, Color{222, 186, 112, static_cast<unsigned char>(40 + 50 * pulso)});
    }
}

} // namespace andv
