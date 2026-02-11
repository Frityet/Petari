#pragma once

namespace smgpc::game::title::runtime {

class TriggerChecker {
public:
    TriggerChecker();

    void update(bool input);
    void set_input(bool input);

    [[nodiscard]] bool level() const;
    [[nodiscard]] bool on_trigger() const;
    [[nodiscard]] bool off_trigger() const;

private:
    bool _previous_level {};
    bool _current_level {};
};

}  // namespace smgpc::game::title::runtime
