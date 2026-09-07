#pragma once

class TriggerChecker {
public:
    TriggerChecker();

    void update(bool input);
    void setInput(bool input);
    bool getLevel() const;
    bool getOnTrigger() const;
    bool getOffTrigger() const;

private:
    /* 0x00 */ bool mPrevLevel;
    /* 0x01 */ bool mCurrLevel;
};

