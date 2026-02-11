#include "NerveExecutor.hpp"

#include <stdexcept>

#include "Spine.hpp"

namespace smgpc::game::title::runtime {

NerveExecutor::NerveExecutor() = default;
NerveExecutor::~NerveExecutor() = default;

void NerveExecutor::init_nerve(const Nerve *nerve) {
    _spine = std::make_unique<Spine>(this, nerve);
}

void NerveExecutor::update_nerve() {
    if (_spine != nullptr) {
        _spine->update();
    }
}

void NerveExecutor::set_nerve(const Nerve *nerve) {
    if (_spine == nullptr) {
        throw std::runtime_error("NerveExecutor::set_nerve called before init_nerve.");
    }
    _spine->set_nerve(nerve);
}

bool NerveExecutor::is_nerve(const Nerve *nerve) const {
    if (_spine == nullptr) {
        return false;
    }
    return _spine->current_nerve() == nerve;
}

std::int32_t NerveExecutor::nerve_step() const {
    if (_spine == nullptr) {
        return 0;
    }
    return _spine->step();
}

Spine *NerveExecutor::spine() const {
    return _spine.get();
}

}  // namespace smgpc::game::title::runtime
