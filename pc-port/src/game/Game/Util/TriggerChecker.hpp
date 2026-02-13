#pragma once

class TriggerChecker {
public:
    TriggerChecker();

    void update(bool input);
    void setInput(bool input);

    [[nodiscard]] bool getLevel() const;
    [[nodiscard]] bool getOnTrigger() const;
    [[nodiscard]] bool getOffTrigger() const;

private:
    /* 0x00 */ bool mPrevLevel;
    /* 0x01 */ bool mCurrLevel;
};
