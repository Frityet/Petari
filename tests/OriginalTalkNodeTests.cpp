#include "OriginalTalkNodeTests.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/NPC/TalkMessageInfo.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/Screen/MessageTagSkipTagProcessor.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "nw4r/ut/CharStrmReader.h"
#include "nw4r/ut/ResFont.h"
#include "nw4r/ut/TextWriterBase.h"
#include <aurora/exception.hpp>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
struct NodeOwner {
    TalkNodeCtrl node;
    ActorCameraInfo* camera = nullptr;
    ~NodeOwner() { delete[] node._0; delete camera; }
};
}
void verify_original_talk_nodes(MessageHolder& holder) {
    auto& data = *holder.mGameMessageData;
    require(MessageSystem::getSceneMessageData() == &data && data.mFlowBlock,
            "node traversal requires the actual scene alias of complete original game message data");
    std::size_t messages = 0, branches = 0, events = 0, terminals = 0, read_history = 0;
    const char* flow_name = nullptr;
    for (u32 index = 0; index < data.mFlowBlock->mNodeCount; ++index) {
        auto* authored = data.getNode(index);
        NodeOwner owner;
        auto& node = owner.node;
        node._38 = authored; node.mFlowNode = authored; node.mCurrentNode = authored;
        require(node.getCurrentNodeMessage() == (authored->mNodeType == 1 ? authored : nullptr) &&
                    node.getCurrentNodeBranch() == (authored->mNodeType == 2 ? authored : nullptr) &&
                    node.getCurrentNodeEvent() == (authored->mNodeType == 3 ? authored : nullptr),
                "all original current-node predicates retain the exact borrowed resource pointer");
        TalkNode* next = nullptr;
        if (authored->mNodeType == 1 && authored->mNextIdx != 0xffff) next = data.getNode(authored->mNextIdx);
        if (authored->mNodeType == 3 && data.isValidBranchNode(authored->mIndex)) next = data.getBranchNode(authored->mIndex);
        require(node.getNextNode() == next && node.isExistNextNode() == bool(next) &&
                    node.isNextNodeMessage() == (next && next->mNodeType == 1) &&
                    node.getNextNodeBranch() == (next && next->mNodeType == 2 ? next : nullptr) &&
                    node.getNextNodeEvent() == (next && next->mNodeType == 3 ? next : nullptr),
                "all actual authored next-node and sentinel paths agree with original FLW pointer relationships");
        if (!next && authored->mNodeType != 2) ++terminals;
        if (authored->mNodeType == 1) {
            TalkMessageInfo expected;
            data.getMessage(&expected, authored->mGroupID, authored->mIndex);
            // Persistent read bits belong to the actual save owner, covered by
            // save tests. This fixture exercises nodes with no such dependency.
            if (expected.mTalkType == 2 && expected._B != -1) continue;
            node.updateMessage();
            require(node.mMessageInfo._0 == expected._0 && node.mCurrentNodeIdx == authored->mIndex &&
                        node.mNodeData == (next && next->mNodeType == 2 ? s16(next->mIndex) : s16(-1)),
                    "original updateMessage borrows actual MessageData text and records branch/index metadata");
            ++messages;
            if (expected.mTalkType == 2 && expected._B == -1 && read_history == 0) {
                node.readMessage();
                require(node.mHistory.mCount == 1 && node.mHistory.search(authored->mIndex) && node.mMessageInfo.mTalkType == 0,
                        "original readMessage records an event message in its actual history and changes its local talk type");
                node.updateMessage(); node.readMessage();
                require(node.mHistory.mCount == 1 && node.mMessageInfo.mTalkType == 0,
                        "original history prevents a repeated event node from becoming unread again");
                ++read_history;
            }
        } else {
            node.mMessageInfo._0 = reinterpret_cast<u8*>(data.mDataBlock + 1);
            node.updateMessage();
            require(!node.mMessageInfo._0 && node.mCurrentNodeIdx == -1,
                    "non-message traversal clears only the original text pointer and retains previous message index");
            if (authored->mNodeType == 3) ++events;
            if (authored->mNodeType == 2) {
                for (bool left : {true, false}) {
                    const auto branch = authored->mNextGroup + (left ? 0 : 1);
                    if (!data.isValidBranchNode(branch)) continue;
                    auto* target = data.getBranchNode(branch);
                    if (target->mNodeType == 1) {
                        TalkMessageInfo info; data.getMessage(&info, target->mGroupID, target->mIndex);
                        if (info.mTalkType == 2 && info._B != -1) continue;
                    }
                    node.mCurrentNode = authored; node.forwardCurrentBranchNode(left);
                    require(node.mCurrentNode == target, "both original branch choices resolve their authored target pointers");
                    ++branches;
                }
            }
        }
    }
    require(messages && branches && events && terminals && read_history,
            "the real archive exercises messages, both branch choices, event nodes, terminal sentinels and event-read history");
    // Root creation uses actual names and original allocation/camera behavior.
    for (s32 row = 0; row < data.mIDTable->getNumEntries() && !flow_name; ++row) {
        const char* name = nullptr; data.mIDTable->getValue(row, "MessageId", &name);
        auto* root = data.findNode(name); if (!root) continue;
        TalkMessageInfo info; data.getMessage(&info, root->mGroupID, root->mIndex);
        if (info.mTalkType != 0 || info._B != -1) continue;
        flow_name = name;
    }
    require(flow_name, "actual message catalog includes a normal flow root without persistent save flags");
    NodeOwner created;
    created.node.createFlowNodeDirect(nullptr, JMapInfoIter{}, flow_name, &created.camera);
    auto* root = data.findNode(flow_name);
    require(created.node._0 != flow_name && std::strcmp(created.node._0, flow_name) == 0 &&
                created.node._38 == root && created.node.mCurrentNode == root && created.node.mFlowNode == root && created.camera,
            "original root creation owns its copied identifier and fallback camera while retaining actual resource nodes");
    auto& node = created.node;
    TalkNode* alternate = nullptr;
    for (u32 i = 0; i < data.mFlowBlock->mNodeCount && !alternate; ++i) {
        auto* candidate = data.getNode(i);
        if (candidate != root && candidate->mNodeType != 1) alternate = candidate;
    }
    require(alternate, "actual graph contains an alternate event or branch state");
    node.mCurrentNode = alternate; node.recordTempFlowNode();
    node.mCurrentNode = root; node.resetTempFlowNode();
    require(node.mCurrentNode == alternate && node.mFlowNode == alternate, "temporary reset returns to the recorded actual node");
    node.resetFlowNode();
    require(node.mCurrentNode == root && node.mFlowNode == root, "full reset returns both original cursors to the authored root");
    std::array<wchar_t, 6> sub{0x1a, 0x0a08, 0, 0, root->mIndex, 0};
    node.mMessageInfo._0 = reinterpret_cast<u8*>(sub.data());
    TalkMessageInfo expected; data.getMessage(&expected, 0, root->mIndex);
    require(node.getSubMessage() == reinterpret_cast<const wchar_t*>(expected._0),
            "original group-8 tag lookup decodes the native wchar parameter and returns actual MessageHolder text");
    sub[1] = 0x0a07; require(!node.getSubMessage(), "another tag group does not fabricate a submessage");
    sub[0] = 'A'; require(!node.getSubMessage(), "ordinary text has no submessage");
    std::cout << "[ok] original talk nodes: messages=" << messages << " branches=" << branches << " events=" << events
              << " terminals=" << terminals << " history=" << read_history << '\n';
}
void verify_original_message_tag_processor(const nw4r::ut::Font& metrics_font) {
    using namespace nw4r::ut;
    TextWriterBase<wchar_t> writer;
    writer.SetFont(metrics_font);
    MessageTagSkipTagProcessor processor;
    std::array<wchar_t, 5> text{0x0806, 0, 0x1234, 'Z', 0};
    PrintContext<wchar_t> context{&writer, text.data(), 10, 0, 0};
    Rect rect(1, 2, 3, 4);
    require(processor.Process(0x1a, &context) == TagProcessorBase<wchar_t>::OPERATION_DEFAULT && context.str == text.data() + 3,
            "original tag processor skips packed-byte length in native code units including zero parameters");
    context.str = text.data();
    require(processor.CalcRect(&rect, 0x1a, &context) == TagProcessorBase<wchar_t>::OPERATION_DEFAULT &&
                context.str == text.data() + 3 && rect.left == 1 && rect.top == 2 && rect.right == 3 && rect.bottom == 4,
            "tag-only rectangle processing advances the stream without changing geometric bounds");
    writer.SetLineSpace(12 - writer.GetFontHeight()); writer.SetCursor(17, 5);
    require(processor.Process('\n', &context) == TagProcessorBase<wchar_t>::OPERATION_NEXT_LINE &&
                writer.GetCursorX() == 10 && writer.GetCursorY() == 17,
            "ordinary line feeds dispatch into the full original NW4R writer and use origin plus line height");
    writer.mIsWidthFixed = true; writer.mFixedWidth = 5; writer.SetTabWidth(4); writer.SetCursorX(17);
    require(processor.CalcRect(&rect, '\t', &context) == TagProcessorBase<wchar_t>::OPERATION_NO_CHAR_SPACE &&
                writer.GetCursorX() == 30 && rect.left == 17 && rect.right == 30 && rect.top == 17 &&
                rect.bottom == 17 + writer.GetFontHeight(),
            "ordinary tabs preserve original fixed-width tab stops and normalized rectangle bounds");
    std::array<wchar_t, 6> params{0x0e08, 0, 0x1234, 0x5678, 0xabcd, 0xef01};
    MessageEditorMessageTag tag(params.data());
    require(tag.getParam32(0) == 0x12345678 && tag.getParam32(1) == 0xabcdef01 && tag.getSkipLength() == 6,
            "recovered tag parameters combine original big-endian words at both u32 positions");
    ResFont font;
    font.InitReaderFunc(FONT_ENCODING_UTF16);
    auto reader = font.GetCharStrmReader();
    std::array<wchar_t, 3> wide{0x3042, 0xd83d, 0}; reader.Set(wide.data());
    require(reader.Next() == 0x3042 && reader.Next() == 0xd83d && reader.GetCurrentPos() == wide.data() + 2,
            "actual font reader consumes native-wide text as retained original code units");
    std::array<u16, 2> raw{0x1234, 0xabcd}; reader.Set(reinterpret_cast<const char*>(raw.data()));
    require(reader.Next() == 0x1234 && reader.Next() == 0xabcd && reader.GetCurrentPos() == raw.data() + 2,
            "explicit raw UTF-16 input retains its two-byte native scalar width");
    font.InitReaderFunc(FONT_ENCODING_SJIS);
    auto sjis_reader = font.GetCharStrmReader();
    sjis_reader.Set("\x91\xcc" "A");
    require(sjis_reader.Next() == 0x91cc && sjis_reader.Next() == 'A', "original SJIS font reader retains encoded two-byte character identity");
    std::cout << "[ok] original tag skip/default writer dispatch, native parameter widths and actual font readers\n";
}
