#pragma once

// Diagnostic-only expression of the smallest retail GX source-compatibility
// tranche. Production ownership belongs in Aurora's GX headers/implementation.
#define _GXAttr GXAttr
#define _GXTlutSize GXTlutSize

inline void smgpcProbeGXSetArrayRetail(GXAttr attr, const void* data, u8 stride) {
    GXSetArray(attr, data, 0, stride, false);
}

#define GXSetArray(attr, data, stride) smgpcProbeGXSetArrayRetail((attr), (data), (stride))
