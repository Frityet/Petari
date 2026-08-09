#include "Game/Util/ScreenUtil.hpp"

#include "Game/Screen/InformationMessage.hpp"
#include "compat/InformationMessageCompat.hpp"
#include "compat/MessageUtilCompat.hpp"

namespace {

    void appear_information_message(bool has_button_layout, bool is_center) {
        auto& message = smgpc::compat::require_information_message();
        message.setCenter(is_center);
        if (has_button_layout) {
            message.appearWithButtonLayout();
        } else {
            message.appear();
        }
    }

}  // namespace

namespace MR {

    void appearInformationMessage(const char* message_id,
                                  bool has_button_layout) {
        smgpc::compat::require_information_message().setMessage(message_id);
        appear_information_message(has_button_layout, false);
    }

    void appearInformationMessageCenter(const char* message_id,
                                        bool has_button_layout) {
        smgpc::compat::require_information_message().setMessage(message_id);
        appear_information_message(has_button_layout, true);
    }

    void appearInformationMessage(const wchar_t* message,
                                  bool has_button_layout) {
        auto& information_message =
            smgpc::compat::require_information_message();
        if (const auto* message_id =
                smgpc::compat::layout_message_id_for_pointer(message);
            message_id != nullptr) {
            information_message.setMessage(message_id);
        } else {
            information_message.setMessage(message);
        }
        appear_information_message(has_button_layout, false);
    }

    void setInformationMessageReplaceString(const wchar_t* message,
                                            s32 index) {
        smgpc::compat::require_information_message().setReplaceString(message,
                                                                      index);
    }

    void disappearInformationMessage() {
        smgpc::compat::require_information_message().disappear();
    }

}  // namespace MR
