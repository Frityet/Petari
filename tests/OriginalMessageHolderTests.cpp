#include "Game/System/MessageHolder.hpp"
#include "Game/NPC/TalkMessageInfo.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/MessageUtilCompat.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/NativeBmgResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/MessageHolderOwnership.hpp"
#include "runtime/RuntimeServices.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/exception.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace aurora { extern AuroraConfig g_config; }
namespace {
using Bytes = std::vector<std::uint8_t>;
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
template<class F> void rejects(F call, const char* message) {
    bool rejected = false;
    try { call(); } catch (const std::exception&) { rejected = true; }
    require(rejected, message);
}
void put16(Bytes& b, std::size_t p, std::uint16_t v) { b.at(p) = v >> 8; b.at(p + 1) = v; }
void put32(Bytes& b, std::size_t p, std::uint32_t v) { put16(b, p, v >> 16); put16(b, p + 2, v); }
void text(Bytes& b, std::size_t p, std::string_view s) { std::memcpy(b.data() + p, s.data(), s.size()); }

Bytes ids() {
    // Sorted raw identifiers: ASCII, then CP932 'body'. No UTF-8 reinterpretation.
    const std::array names{std::string("Alias"), std::string("Tagged"), std::string("\x91\xcc", 2)};
    Bytes b(0x28 + names.size() * 8);
    put32(b, 0, names.size()); put32(b, 4, 2); put32(b, 8, 0x28); put32(b, 12, 8);
    put32(b, 16, smgpc::resource::jmap_hash("MessageId")); put32(b, 20, 0xffffffff); b[27] = 6;
    put32(b, 28, smgpc::resource::jmap_hash("Index")); put32(b, 32, 0xffffffff); put16(b, 36, 4);
    const auto strings = b.size();
    for (std::size_t i = 0; i < names.size(); ++i) {
        put32(b, 0x28 + i * 8, b.size() - strings); put32(b, 0x2c + i * 8, i);
        b.insert(b.end(), names[i].begin(), names[i].end()); b.push_back(0);
    }
    return b;
}
Bytes bmg() {
    Bytes b(0x20); text(b, 0, "MESGbmg1"); b[16] = 2; put32(b, 12, 3);
    const auto inf = b.size(); b.resize(b.size() + 16 + 3 * 12); text(b, inf, "INF1");
    put32(b, inf + 4, b.size() - inf); put16(b, inf + 8, 3); put16(b, inf + 10, 12);
    for (unsigned i = 0; i < 3; ++i) {
        const auto p = inf + 16 + i * 12;
        put32(b, p, i == 1 ? 2 : 0); put16(b, p + 4, 0x1234 + i);
        b[p + 6] = 0xff; b[p + 7] = 2; b[p + 8] = 4; b[p + 9] = 3;
        b[p + 10] = 0x80; b[p + 11] = 0x7f;
    }
    const auto dat = b.size();
    // Message 0 and 2 alias the same empty string; message 1 includes a zero
    // inside a control tag and an uncombined surrogate pair.
    constexpr std::array<std::uint16_t, 10> units{0, 'A', 0x1a, 0x0806, 0, 0x1234, 0xd83d, 0xde00, 'Z', 0};
    b.resize(dat + 8 + units.size() * 2); text(b, dat, "DAT1"); put32(b, dat + 4, b.size() - dat);
    for (std::size_t i = 0; i < units.size(); ++i) put16(b, dat + 8 + i * 2, units[i]);
    const auto flow = b.size(); b.resize(flow + 16 + 3 * 8 + 2 * 2);
    text(b, flow, "FLW1"); put32(b, flow + 4, b.size() - flow); put16(b, flow + 8, 3); put16(b, flow + 10, 2);
    b[flow + 16] = 1; b[flow + 17] = 7; put16(b, flow + 18, 1); put16(b, flow + 20, 2);
    b[flow + 24] = 2; b[flow + 25] = 9; put16(b, flow + 26, 0x2345); put16(b, flow + 28, 0);
    b[flow + 32] = 3; b[flow + 33] = 4; put16(b, flow + 34, 0); put32(b, flow + 36, 0x12345678);
    put16(b, flow + 40, 1); put16(b, flow + 42, 0xffff);
    put32(b, 8, b.size()); return b;
}
Bytes archive(const Bytes& messages, const Bytes& identifiers) {
    constexpr std::string_view strings{"root\0Message.bmg\0MessageId.tbl\0", 31};
    constexpr std::size_t dirs = 0x40, files = 0x50, names = 0x78, payload = 0xa0;
    const auto second = (messages.size() + 31) & ~std::size_t{31};
    Bytes b(payload + second + identifiers.size());
    text(b, 0, "RARC"); put32(b, 4, b.size()); put32(b, 8, 0x20); put32(b, 12, payload - 0x20);
    put32(b, 16, b.size() - payload); put32(b, 0x20, 1); put32(b, 0x24, dirs - 0x20);
    put32(b, 0x28, 2); put32(b, 0x2c, files - 0x20); put32(b, 0x30, strings.size()); put32(b, 0x34, names - 0x20);
    text(b, dirs, "ROOT"); put16(b, dirs + 10, 2); text(b, names, strings);
    for (unsigned i = 0; i < 2; ++i) {
        const auto p = files + i * 20; put16(b, p, i);
        put32(b, p + 4, 0x11000000 | (i ? 17 : 5)); put32(b, p + 8, i ? second : 0);
        put32(b, p + 12, i ? identifiers.size() : messages.size());
    }
    std::copy(messages.begin(), messages.end(), b.begin() + payload);
    std::copy(identifiers.begin(), identifiers.end(), b.begin() + payload + second);
    return b;
}
void native_boundary(smgpc::resource::GameResourceRuntime& process, smgpc::runtime::ArchiveMountService& mounts) {
    const auto message_bytes = bmg(), identifier_bytes = ids();
    auto domain = smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 1U << 20);
    (void)mounts.mount_memory("/NativeMessageFixture.arc", archive(message_bytes, identifier_bytes), &domain->heap());
    {
        smgpc::compat::JkrAllocationScope heap(domain);
        MessageData data("/NativeMessageFixture.arc");
        TalkMessageInfo info;
        require(data.getMessageDirect(&info, "Tagged") && data.findMessageIndex("\x91\xcc") == 2 &&
                    data.findMessageIndex("\xe4\xbd\x93") == -1,
                "original JMapInfo lookup preserves exact CP932 identity and rejects the UTF-8 spelling");
        require(info.mCameraSetID == 0x1235 && info._6 == -1 && info._A == -128 && info._B == 127 &&
                    info.mCameraType == 2 && info.mTalkType == 4 && info.mBalloonType == 3,
                "original information fields read native camera scalars and unchanged signed metadata bytes");
        const auto* wide = reinterpret_cast<const wchar_t*>(info._0);
        constexpr std::array<wchar_t, 9> expected{'A', 0x1a, 0x0806, 0, 0x1234, 0xd83d, 0xde00, 'Z', 0};
        require(std::equal(expected.begin(), expected.end(), wide),
                "DAT1 widening keeps zero tag parameters, surrogate code units and the original terminator");
        require(data.mNativeResource->message(0) == data.mNativeResource->message(2) &&
                    wide == data.mNativeResource->message(0) + 1 &&
                    JKRHeap::findFromRoot(data.mNativeResource->data()) == nullptr,
                "native byte-offset relocation preserves aliases/interior offsets in retained host storage");
        require(data.findNode("Tagged") == data.getNode(0) && data.getNode(0)->mNextIdx == 2 &&
                    data.getNode(2)->mUnknown == 0x12345678 && data.getBranchNode(0) == data.getNode(1) &&
                    data.isValidBranchNode(0) && !data.isValidBranchNode(1),
                "original flow methods retain node, branch, sentinel and event-u32 union semantics");
    }
    mounts.remove_for_heap(&domain->heap());
    auto malformed = message_bytes;
    put32(malformed, 0x30, 1);
    rejects([&] { smgpc::resource::NativeBmgResource invalid(malformed, identifier_bytes); },
            "odd original UTF-16 byte offsets cannot enter unchecked original field access");
    malformed = message_bytes;
    put16(malformed, malformed.size() - 4, 3);
    rejects([&] { smgpc::resource::NativeBmgResource invalid(malformed, identifier_bytes); },
            "out-of-range flow branches are rejected before original pointer arithmetic");
    malformed = message_bytes;
    put32(malformed, 0x30, 0x7ffffffe);
    rejects([&] { smgpc::resource::NativeBmgResource invalid(malformed, identifier_bytes); },
            "text offsets outside retained DAT1 data are rejected before pointer publication");
    malformed = message_bytes;
    const auto parsed = smgpc::resource::BmgMessageArchive::from_bytes(message_bytes, identifier_bytes);
    const auto* original_dat = parsed.block("DAT1");
    malformed.insert(malformed.end(), message_bytes.begin() + original_dat->offset,
                     message_bytes.begin() + original_dat->offset + original_dat->available_size);
    put32(malformed, 8, malformed.size()); put32(malformed, 12, 4);
    rejects([&] { smgpc::resource::NativeBmgResource invalid(malformed, identifier_bytes); },
            "duplicate typed blocks cannot split original block lookup from retained native text identity");
    std::cout << "[ok] original MessageData native scalar/text/CP932 identity/flow boundary\n";
}
std::size_t compare_data(MessageData& data, const smgpc::resource::RarcArchive& archive) {
    const auto source = smgpc::resource::BmgMessageArchive::from_message_archive(archive);
    const auto table = smgpc::resource::BcsvTable::from_bytes(archive.resource_data("MessageId.tbl"));
    require(data.mInfoBlock->mItemCount == source.message_count(), "original table count matches authored INF1");
    for (std::size_t row = 0; row < table.entry_count(); ++row) {
        const auto id = table.get_string(row, "MessageId"); const auto index = table.get_s32(row, "Index");
        require(id && index && data.findMessageIndex(id->c_str()) == *index, "every raw authored ID resolves through actual JMapInfo");
        TalkMessageInfo info; require(data.getMessageDirect(&info, id->c_str()), "every original authored message lookup succeeds");
        const auto& expected = source.messages()[*index]; const auto* wide = reinterpret_cast<const wchar_t*>(info._0);
        require(info.mCameraSetID == expected.info.camera_set_id && info.mCameraType == expected.info.camera_type &&
                    info.mTalkType == expected.info.talk_type && info.mBalloonType == expected.info.balloon_type &&
                    static_cast<u8>(info._6) == expected.info.unknown_06 && static_cast<u8>(info._A) == expected.info.unknown_0a &&
                    static_cast<u8>(info._B) == expected.info.unknown_0b,
                "all actual original message metadata matches independently decoded Wii bytes");
        require(std::equal(expected.raw_text.begin(), expected.raw_text.end(), wide) && wide[expected.raw_text.size()] == 0,
                "all original raw message code units and terminators match retained authored text");
    }
    if (source.flow()) {
        const auto& flow = *source.flow();
        require(data.mFlowBlock && data.mFlowBlock->mNodeCount == flow.node_count && data.mFlowBlock->_A == flow.branch_count,
                "complete original flow table retains authored node and branch counts");
        for (std::size_t i = 0; i < flow.nodes.size(); ++i) {
            const auto* node = data.getNode(i); const auto& expected = flow.nodes[i];
            require(node->mNodeType == expected.node_type && node->mGroupID == expected.group_id && node->mIndex == expected.index &&
                        (node->mNodeType == 3 ? node->mUnknown == expected.raw_next :
                         node->mNextIdx == expected.next_index && node->mNextGroup == expected.next_group),
                    "every actual flow node reads original native scalar fields with event union width preserved");
        }
        for (std::size_t i = 0; i < flow.branch_node_indices.size(); ++i) {
            const auto index = flow.branch_node_indices[i];
            require(data.isValidBranchNode(i) == (index != 0xffff), "all original branch sentinel predicates agree");
            if (index != 0xffff) require(data.getBranchNode(i) == data.getNode(index), "all original branch pointers resolve to their exact nodes");
        }
    }
    return table.entry_count();
}
void original_holder(smgpc::resource::GameResourceRuntime& process, smgpc::runtime::ArchiveMountService& mounts) {
    const auto mounts_before = mounts.size();
    const auto free_before = process.host_heaps()->root_heap().getTotalFreeSize();
    for (int generation = 0; generation < 2; ++generation) {
        std::weak_ptr<JMapInfo::DataCompat> metadata;
        {
            smgpc::runtime::MessageHolderOwnership owner(process.host_heaps(), process.budget().message_resource_bytes,
                mounts, "/KrKorean/MessageData/Message.arc", "KrKorean");
            auto& holder = owner.holder(); metadata = holder.mGameMessageData->mIDTable->mData;
            require(smgpc::runtime::current_message_holder() == &holder && holder.mSceneMessageData == nullptr &&
                        holder.mSystemMessageData != holder.mGameMessageData,
                    "one complete actual holder publishes separate system and game owners before scene initialization");
            const auto& embedded = mounts.retain("ErrorMessageArchive.arc")->source();
            const auto* korean = embedded.find_normalized("krkorean/messagedata/system.arc");
            require(korean && embedded.find_resource("/KrKorean/MessageData/System.arc") == korean &&
                        !embedded.find_resource("/AbsentLanguage/MessageData/System.arc"),
                    "qualified JKR resource paths fold case without falling through to a different language");
            const auto compressed = embedded.file_data(*korean);
            const auto canonical = smgpc::resource::RarcArchive::from_bytes(Bytes(compressed.begin(), compressed.end()));
            const auto mounted_system = mounts.retain("/Memory/SystemMessage.arc")->source().bytes();
            require(std::ranges::equal(canonical.bytes(), mounted_system),
                    "the original system owner mounts the exact selected language resource from the embedded archive");
            const auto system_count = compare_data(*holder.mSystemMessageData, mounts.retain("/Memory/SystemMessage.arc")->source());
            const auto game_count = compare_data(*holder.mGameMessageData, mounts.retain("/MessageData/Message.arc")->source());
            holder.initSceneData(); require(MessageSystem::getSceneMessageData() == holder.mGameMessageData,
                                            "original scene initialization aliases the actual game message data");
            holder.destroySceneData(); require(!MessageSystem::getSceneMessageData() && holder.mGameMessageData,
                                               "original scene retirement clears the alias without deleting process messages");
            const char* id = nullptr;
            require(holder.mGameMessageData->mIDTable->getValue(0, "MessageId", &id), "the first game ID is authored");
            const auto* message = MR::getGameMessageDirect(id);
            require(message && message == MR::getLayoutMessageDirect(id) && MR::getGameMessageDirectUtf16(id) &&
                        MR::isExistGameMessage(id) && smgpc::compat::layout_message_id_for_pointer(message),
                    "shared Game and layout utilities read retained actual MessageData pointers");
            const char* system_id = nullptr;
            require(holder.mSystemMessageData->mIDTable->getValue(0, "MessageId", &system_id), "the first system ID is authored");
            TalkMessageInfo info;
            require(MessageSystem::getSystemMessageDirect(&info, system_id) &&
                        MR::getSystemMessageDirect(system_id) == reinterpret_cast<const wchar_t*>(info._0),
                    "system utilities use original embedded system data rather than game-text fallback");
            const auto before = info;
            require(!MessageSystem::getGameMessageDirect(&info, "Missing_Actual_Message") && info._0 == before._0 &&
                        !MR::getGameMessageDirect("Missing_Actual_Message") && !MR::getGameMessageDirect(nullptr),
                    "original failed lookup preserves the caller record and utility absence stays null");
            rejects([&] { smgpc::runtime::MessageHolderOwnership duplicate(process.host_heaps(), 1U << 20,
                mounts, "/KrKorean/MessageData/Message.arc", "KrKorean"); }, "a second process owner cannot replace live message identity");
            std::cout << "[ok] actual MessageHolder generation " << generation << ": system=" << system_count << " game=" << game_count << '\n';
        }
        require(metadata.expired() && mounts.size() == mounts_before && !smgpc::runtime::current_message_holder() &&
                    process.host_heaps()->root_heap().getTotalFreeSize() == free_before,
                "holder teardown releases native table metadata, both mounts and its complete original Game heap");
    }
}
}
int main() {
    try {
        require(!MR::getGameMessageDirect("Missing") && !MR::getSystemMessageDirect("Missing") &&
                    !MR::getLayoutMessageDirect("Missing") && !MR::isExistGameMessage("Missing"),
                "utility absence never manufactures an owner");
        rejects([] { (void)MessageSystem::getSceneMessageData(); }, "direct original access requires an actual holder");
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && aurora_dvd_open(disc), "SMGPC_REAL_DISC must identify the original disc for complete message proof");
        struct CloseDisc { ~CloseDisc() { aurora_dvd_close(); } } close;
        DVDInit(); aurora::g_config.mem1Size = 24U << 20;
        smgpc::resource::GameResourceRuntime process;
        smgpc::runtime::DvdFileSystemService dvd({});
        smgpc::runtime::ArchiveMountService mounts(dvd);
        native_boundary(process, mounts);
        original_holder(process, mounts);
        std::cout << "[ok] original message owner, all authored records, shared getters and repeated teardown\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "[failed] " << error.what() << '\n'; return 1; }
}
