#include "andvarion/core/EventBus.hpp"

#include <algorithm>

namespace andv {

Inscricao& Inscricao::operator=(Inscricao&& o) noexcept {
    if (this != &o) {
        cancelar();
        barramento_ = std::move(o.barramento_);
        id_         = o.id_;
        o.id_       = 0;
    }
    return *this;
}

void Inscricao::cancelar() {
    if (id_ == 0) return;
    if (auto b = barramento_.lock()) (*b)->remover(id_);
    id_ = 0;
    barramento_.reset();
}

EventBus::EventBus() : vivo_(std::make_shared<EventBus*>(this)) {}
EventBus::~EventBus() = default;  // vivo_ morre aqui: inscrições restantes viram no-op

Inscricao EventBus::inscrever(TipoEvento tipo, Observador obs) {
    const u32 id = proximoId_++;
    registros_.push_back({id, false, tipo, std::move(obs)});
    return Inscricao(vivo_, id);
}

Inscricao EventBus::inscreverTodos(Observador obs) {
    const u32 id = proximoId_++;
    registros_.push_back({id, true, TipoEvento::InimigoMorto, std::move(obs)});
    return Inscricao(vivo_, id);
}

void EventBus::remover(u32 id) {
    // Durante a entrega só anulamos o callback; a compactação vem depois.
    for (auto& r : registros_)
        if (r.id == id) r.obs = nullptr;
    if (!entregando_)
        registros_.erase(std::remove_if(registros_.begin(), registros_.end(),
                                        [](const Registro& r) { return !r.obs; }),
                         registros_.end());
}

void EventBus::publicar(const Evento& e) {
    fila_.push_back(e);
    if (entregando_) return;  // será entregue pelo laço externo

    entregando_ = true;
    for (std::size_t i = 0; i < fila_.size(); ++i) {
        const Evento atual = fila_[i];  // cópia: fila_ pode realocar
        // Itera por índice: observadores podem se inscrever durante a entrega.
        for (std::size_t k = 0; k < registros_.size(); ++k) {
            if (!registros_[k].obs) continue;
            if (registros_[k].todos || registros_[k].tipo == atual.tipo) {
                auto obs = registros_[k].obs;  // cópia: o registro pode ser anulado
                obs(atual);
            }
        }
    }
    fila_.clear();
    entregando_ = false;
    registros_.erase(std::remove_if(registros_.begin(), registros_.end(),
                                    [](const Registro& r) { return !r.obs; }),
                     registros_.end());
}

} // namespace andv
