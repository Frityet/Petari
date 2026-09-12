#pragma once

namespace smgpc::runtime { class SaveDataService; }

namespace smgpc::compat {
    // Publish the process NAND/storage boundary independently of Game owners.
    // Stop original NAND workers before retiring this borrowed service.
    class NandSdkBinding final {
    public:
        explicit NandSdkBinding(runtime::SaveDataService&);
        ~NandSdkBinding();
        NandSdkBinding(const NandSdkBinding&) = delete;
        NandSdkBinding& operator=(const NandSdkBinding&) = delete;

    private:
        runtime::SaveDataService* _service;
    };
}
