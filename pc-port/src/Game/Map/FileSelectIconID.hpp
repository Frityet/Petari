#pragma once

#include <revolution.h>

class FileSelectIconID {
public:
    enum EFellowID {
        Mario,
        Luigi,
        Yoshi,
        Kinopio,
        Peach,
    };

    FileSelectIconID();
    FileSelectIconID(const FileSelectIconID& rOther);

    bool operator==(const FileSelectIconID& rOther) const {
        return mIsMii == rOther.mIsMii && mData == rOther.mData;
    }

    bool operator!=(const FileSelectIconID& rOther) const {
        return !(*this == rOther);
    }

    void set(const FileSelectIconID& rOther);
    void setMiiIndex(u16 miiIndex);
    [[nodiscard]] bool isMii() const;
    [[nodiscard]] u16 getMiiIndex() const;
    void setFellowID(EFellowID fellowID);
    [[nodiscard]] bool isFellow() const;
    [[nodiscard]] EFellowID getFellowID() const;

private:
    /* 0x0 */ bool mIsMii;
    /* 0x2 */ u16 mData;
};
