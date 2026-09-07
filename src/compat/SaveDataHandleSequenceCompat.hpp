#pragma once

class SaveDataHandleSequence;

namespace smgpc::runtime {
class RuntimeContext;
}

namespace smgpc::compat {
// The host deliberately cannot construct a partial save-data core while the
// retail GameDataHolder chunk closure is absent.
[[noreturn]] void ensure_save_data_core_initialized(SaveDataHandleSequence& sequence);

[[nodiscard]] bool try_initialize_save_data_ui(SaveDataHandleSequence& sequence,
                                               smgpc::runtime::RuntimeContext& runtime);
[[nodiscard]] bool try_initialize_save_data_ui(SaveDataHandleSequence& sequence);
}  // namespace smgpc::compat

namespace smgpc::game {
[[noreturn]] SaveDataHandleSequence& save_data_handle_sequence();
}
