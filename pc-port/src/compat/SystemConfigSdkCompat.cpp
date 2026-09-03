#include "runtime/SystemConfigService.hpp"

namespace {
    struct InterruptScope {
        BOOL enabled = OSDisableInterrupts();
        ~InterruptScope() { OSRestoreInterrupts(enabled); }
    };
}

extern "C" {
BOOL SCFindByteArrayItem(void* data, u32 size, SCItemID id) {
    InterruptScope interrupts;
    return smgpc::runtime::SystemConfigService::require_active().find_array(data, size, id);
}
BOOL SCFindU8Item(u8* data, SCItemID id) {
    InterruptScope interrupts;
    return smgpc::runtime::SystemConfigService::require_active().find_integer(data, id, aurora::SysConf::Type::Byte);
}
BOOL SCFindS8Item(s8* data, SCItemID id) {
    InterruptScope interrupts;
    return smgpc::runtime::SystemConfigService::require_active().find_integer(data, id, aurora::SysConf::Type::Byte);
}
BOOL SCFindU32Item(u32* data, SCItemID id) {
    InterruptScope interrupts;
    return smgpc::runtime::SystemConfigService::require_active().find_integer(data, id, aurora::SysConf::Type::Long);
}
BOOL SCReplaceByteArrayItem(const void* data, u32 size, SCItemID id) {
    InterruptScope interrupts;
    return smgpc::runtime::SystemConfigService::require_active().replace_array(data, size, id);
}
BOOL SCReplaceU8Item(u8 value, SCItemID id) {
    InterruptScope interrupts;
    return smgpc::runtime::SystemConfigService::require_active().replace_u8(value, id);
}
}
