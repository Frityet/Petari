#pragma once

namespace smgpc::game::title::runtime {

class TriggerChecker {
public:
    TriggerChecker();

    void update(bool input);
    void setInput(bool input);

    [[nodiscard]] bool getLevel() const;
    [[nodiscard]] bool getOnTrigger() const;
    [[nodiscard]] bool getOffTrigger() const;

private:
    bool mPrevLevel {};
    bool mCurrLevel {};
};

}  // namespace smgpc::game::title::runtime
