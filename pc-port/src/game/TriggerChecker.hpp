#pragma once

namespace pcport::game {

// Copied from include/Game/Util/TriggerChecker.hpp with namespace adaptation.
class TriggerChecker {
public:
    TriggerChecker();

    void update(bool input);
    void setInput(bool input);

    bool getLevel() const;
    bool getOnTrigger() const;
    bool getOffTrigger() const;

private:
    bool mPrevLevel;
    bool mCurrLevel;
};

}  // namespace pcport::game
