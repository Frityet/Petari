#include "TriggerChecker.hpp"

namespace smgpc::game::title::runtime {

TriggerChecker::TriggerChecker()
    : _previous_level(false), _current_level(false) {
}

void TriggerChecker::update(bool input) {
    _previous_level = _current_level;
    _current_level = input;
}

void TriggerChecker::set_input(bool input) {
    _previous_level = input;
    _current_level = input;
}

bool TriggerChecker::level() const {
    return _current_level;
}

bool TriggerChecker::on_trigger() const {
    return (not _previous_level) and _current_level;
}

bool TriggerChecker::off_trigger() const {
    return _previous_level and (not _current_level);
}

}  // namespace smgpc::game::title::runtime
