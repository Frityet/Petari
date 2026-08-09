#pragma once

#include <memory>

class InformationMessage;

namespace smgpc::compat {

    // Scene-scoped owner for the exact InformationMessage and the retail
    // IconAButton child it allocates with raw new. Construction must happen
    // before scene movement begins so ScreenUtil never mutates the scheduler
    // from inside InformationObserver::exeDisp.
    class InformationMessageBinding final {
    public:
        InformationMessageBinding();
        ~InformationMessageBinding();

        InformationMessageBinding(const InformationMessageBinding&) = delete;
        InformationMessageBinding& operator=(const InformationMessageBinding&) = delete;
        InformationMessageBinding(InformationMessageBinding&&) = delete;
        InformationMessageBinding& operator=(InformationMessageBinding&&) = delete;

        [[nodiscard]] InformationMessage& message();
        [[nodiscard]] const InformationMessage& message() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    [[nodiscard]] InformationMessage* current_information_message() noexcept;
    [[nodiscard]] InformationMessage& require_information_message();

}  // namespace smgpc::compat
