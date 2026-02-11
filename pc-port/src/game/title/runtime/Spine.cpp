#include "Spine.hpp"

#include "Nerve.hpp"

namespace smgpc::game::title::runtime {

Spine::Spine(void *executor, const Nerve *nerve)
    : _executor(executor), _current_nerve(nerve), _next_nerve(nullptr), _step(0) {
}

void Spine::update() {
    change_nerve();
    if (_current_nerve != nullptr) {
        _current_nerve->execute(this);
    }
    ++_step;
    change_nerve();
}

void Spine::set_nerve(const Nerve *nerve) {
    if (_current_nerve != nullptr && _step >= 0) {
        _current_nerve->execute_on_end(this);
    }

    _next_nerve = nerve;
    _step = -1;
}

const Nerve *Spine::current_nerve() const {
    if (_next_nerve != nullptr) {
        return _next_nerve;
    }
    return _current_nerve;
}

void *Spine::executor() const {
    return _executor;
}

std::int32_t Spine::step() const {
    return _step;
}

void Spine::change_nerve() {
    if (_next_nerve == nullptr) {
        return;
    }

    _current_nerve = _next_nerve;
    _next_nerve = nullptr;
    _step = 0;
}

}  // namespace smgpc::game::title::runtime
