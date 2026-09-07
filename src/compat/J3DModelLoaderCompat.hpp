#pragma once

class J3DModelData;
struct J3DShapeBlock;

namespace smgpc::compat {
    // Original loader finalization over retained, validated native components.
    void finalize_j3d_model(J3DModelData&, const J3DShapeBlock&, bool binary_display_list);
}
