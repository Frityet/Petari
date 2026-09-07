#pragma once

#include "Game/System/GameDataPlayerStatus.hpp"

namespace smgpc::compat {

// The selected profile owns this concrete value. Game borrows its original
// base pointer; destruction never deletes through BinaryDataChunkBase.
class PlayerStatusStorage final : public GameDataPlayerStatus {
public:
    s32 serialize(u8* buffer, u32 size) const override;
    s32 deserialize(const u8* buffer, u32 size) override;
};

}  // namespace smgpc::compat
