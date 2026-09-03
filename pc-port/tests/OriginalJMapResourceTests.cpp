#include "Game/Util/JMapInfo.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"

#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace {
    constexpr std::string_view name = "AnimationNameLongEnoughToRequireRetainedAllocatedStorage";
    constexpr std::string_view inline_name = "inline-animation";
    void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    void put32(std::vector<u8>& out, std::size_t offset, u32 value) {
        out[offset] = value >> 24; out[offset + 1] = value >> 16;
        out[offset + 2] = value >> 8; out[offset + 3] = value;
    }
    auto fixture() {
        std::vector<u8> out(40 + 36 + name.size() + 1);
        put32(out, 0, 1); put32(out, 4, 2); put32(out, 8, 40); put32(out, 12, 36);
        put32(out, 16, smgpc::resource::jmap_hash("inline")); put32(out, 20, 0xffffffff);
        out[27] = 1;
        put32(out, 28, smgpc::resource::jmap_hash("name")); put32(out, 32, 0xffffffff);
        out[37] = 32; out[39] = 6;
        std::memcpy(out.data() + 40, inline_name.data(), inline_name.size());
        put32(out, 72, 0);
        std::memcpy(out.data() + 76, name.data(), name.size());
        return out;
    }
    const char* read(const smgpc::resource::JMapResource& resource, const char* key) {
        JMapInfo local;
        require(local.attach(resource.data()), "actual unsized attach resolves retained bounded resource");
        const char* result = nullptr;
        require(local.getValue(0, key, &result), "actual string getter reads named field");
        return result;
    }
    void test_reader_lifetime() {
        auto resource = smgpc::resource::JMapResource(fixture());
        const char* borrowed = read(resource, "name");
        const char* short_borrowed = read(resource, "inline");
        require(std::string_view(borrowed) == name && std::string_view(short_borrowed) == inline_name,
                "original-style borrowed strings survive local JMapInfo destruction");
        for (int i = 0; i < 100; ++i) {
            require(read(resource, "name") == borrowed && read(resource, "inline") == short_borrowed,
                    "repeated reads and attaches retain exact character-pointer identity");
        }
    }
    void test_attached_reader_retains_data() {
        JMapInfo survivor;
        const char* borrowed;
        {
            auto resource = smgpc::resource::JMapResource(fixture());
            require(survivor.attach(resource.data()), "survivor attaches real table");
            borrowed = read(resource, "name");
        }
        require(std::string_view(borrowed) == name, "attached reader retains shared string owner after resource release");
        const char* again = nullptr;
        require(survivor.getValue(0, "name", &again) && again == borrowed, "surviving table shares same cache identity");
    }
    void test_rebind_keeps_other_owner() {
        auto first = smgpc::resource::JMapResource(fixture());
        auto second = smgpc::resource::JMapResource(fixture());
        JMapInfo info;
        require(info.attach(first.data()), "first attach");
        const char* borrowed = nullptr;
        require(info.getValue(0, "name", &borrowed), "first borrowed value");
        require(info.attach(second.data()), "second attach");
        const char* rebound = nullptr;
        require(info.getValue(0, "name", &rebound), "second borrowed value");
        require(borrowed != rebound && std::string_view(borrowed) == name && read(first, "name") == borrowed,
                "rebind cannot clear or replace strings retained by the old resource");
    }
    void test_concurrent_readers() {
        auto resource = smgpc::resource::JMapResource(fixture());
        std::array<const char*, 6> pointers{};
        std::array<std::thread, 6> readers;
        for (std::size_t i = 0; i < readers.size(); ++i) {
            readers[i] = std::thread([&, i] {
                for (int repeat = 0; repeat < 100; ++repeat) pointers[i] = read(resource, "name");
            });
        }
        for (auto& reader : readers) reader.join();
        for (auto* pointer : pointers) require(pointer == pointers[0], "concurrent readers share one stable cache entry");
    }
}
int main() {
    const std::array tests{
        std::pair{"borrowed name outlives local reader", test_reader_lifetime},
        std::pair{"attached reader retains released resource", test_attached_reader_retains_data},
        std::pair{"rebind preserves other owner", test_rebind_keeps_other_owner},
        std::pair{"concurrent shared readers", test_concurrent_readers},
    };
    for (const auto& [label, test] : tests) {
        try { test(); std::cout << "PASS " << label << '\n'; }
        catch (const std::exception& error) { std::cerr << "FAIL " << label << ": " << error.what() << '\n'; return 1; }
    }
}
