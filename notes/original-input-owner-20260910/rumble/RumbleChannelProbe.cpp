#include "Game/System/WPadRumble.hpp"
#include "Game/System/WPadRumbleData.hpp"

#include <cstdio>

int main() {
    unsigned checks = 0;
    unsigned failures = 0;
    const auto check = [&](bool value, const char* description) {
        ++checks;
        if (!value) { ++failures; std::printf("FAIL: %s\n", description); }
    };
    RumbleChannel channel;
    RumblePattern pattern = {"Mixed", 3, {2, 0, 3}, 1};
    const int requester = 1;
    const auto cleared = [&] {
        return channel._0 == nullptr && !channel._4 && channel._8 == 0 && channel._C == 0 &&
               !channel._E && channel._10 == nullptr;
    };
    channel.clear();
    check(cleared(), "clear resets every original channel field");
    channel.update();
    check(cleared(), "inactive update retains cleared channel");
    channel.setPattern(&requester, pattern, 42, false);
    check(channel._0 == &pattern && channel._10 == &requester && channel._8 == 42 && channel._C == 0 && !channel._4,
          "setPattern retains caller pattern, requester, and sequence");
    channel.update();
    check(channel._E && channel._C == 1, "first nonzero byte drives motor bit");
    channel.update();
    check(!channel._E && channel._C == 2, "second zero byte stops motor bit");
    channel.update();
    check(channel._E && channel._C == 3, "third nonzero byte resumes motor bit");
    channel.update();
    check(cleared(), "nonlooping pattern clears after its declared frame count");
    channel.setPattern(&requester, pattern, 43, true);
    channel.update();
    channel.update();
    channel.update();
    channel.update();
    check(channel._0 == &pattern && channel._4 && channel._E && channel._C == 1 && channel._8 == 43,
          "looping pattern returns to its first byte without losing ownership");
    channel.update();
    check(!channel._E && channel._C == 2, "loop resumes at second byte");
    channel.clear();
    check(cleared(), "explicit clear releases looping pattern");
    pattern.mPattern[0] = 0;
    pattern.mPattern[1] = 1;
    channel.setPattern(&requester, pattern, 44, false);
    channel.update();
    check(!channel._E && channel._C == 1, "leading zero remains off");
    channel.update();
    check(channel._E && channel._C == 2, "later nonzero turns on after leading zero");
    channel.clear();
    std::printf("Original RumbleChannel: %u/%u checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
