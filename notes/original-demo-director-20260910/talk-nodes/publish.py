from pathlib import Path
root=Path.cwd();note=root/'notes/original-demo-director-20260910/talk-nodes';overlay=note/'headers'
for h in ['ut/TagProcessorBase.h','ut/TextWriterBase.h','ut/CharWriter.h','ut/CharStrmReader.h','math/constant.h']:
 p=root/'src/nw4r'/h;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((overlay/'nw4r'/h).read_bytes())
for h in ['ut/Rect.h','ut/Font.h']:
 (root/'src/nw4r'/h).write_bytes((overlay/'nw4r'/h).read_bytes())
for s in ['ut_TagProcessorBase.cpp','ut_TextWriterBase.cpp','ut_CharWriter.cpp','ut_CharStrmReader.cpp','ut_Font.cpp']:
 (root/'src/nw4r/ut'/s).write_bytes((root/'decomp/src/nw4r/ut'/s).read_bytes())
p=root/'src/nw4r/ut/CharStrmReader.h';s=p.read_text().replace('mReadFunc(rhs.mReadFunc) {}','mReadFunc(rhs.mReadFunc), mNativeWide(rhs.mNativeWide) {}');s=s.replace('mReadFunc(func) {}','mReadFunc(func), mNativeWide(false) {}');s=s.replace('Set(const char* stream) { mCharStrm = stream; }','Set(const char* stream) { mCharStrm = stream; mNativeWide = false; }');s=s.replace('Set(const wchar_t* stream) { mCharStrm = stream; }','Set(const wchar_t* stream) { mCharStrm = stream; mNativeWide = true; }');s=s.replace('const ReadNextCharFunc mReadFunc;','const ReadNextCharFunc mReadFunc;\n            // Native wide text retains original UTF-16 code units at wchar_t width.\n            bool mNativeWide;');p.write_text(s)
p=root/'src/nw4r/ut/ut_CharStrmReader.cpp';s=p.read_text().replace('u16 CharStrmReader::ReadNextCharUTF16() {','u16 CharStrmReader::ReadNextCharUTF16() {\n            if (mNativeWide) {\n                u16 code = static_cast<u16>(GetChar<wchar_t>());\n                StepStrm<wchar_t>();\n                return code;\n            }');p.write_text(s)
p=root/'src/compat/Nw4rFontCompat.cpp';s=p.read_text().replace('mHostResourceState->source_size = declared_size;','mHostResourceState->source_size = declared_size;\n            InitReaderFunc(GetEncoding());');p.write_text(s)
for relative in ['Game/NPC/TalkNodeCtrl.cpp','Game/Screen/MessageTagSkipTagProcessor.cpp']:
 p=root/'src'/relative;p.write_bytes((root/'decomp/src'/relative).read_bytes())
p=root/'src/Game/Screen/MessageTagSkipTagProcessor.hpp';p.write_bytes((root/'decomp/include/Game/Screen/MessageTagSkipTagProcessor.hpp').read_bytes())
p=root/'src/Game/Screen/MessageTagSkipTagProcessor.cpp';s=p.read_text().replace('return (reinterpret_cast< const u8* >(mMessage)[0] - 2U) >> 1;', '''#if defined(TARGET_PC)
    return ((static_cast<u32>(*mMessage) >> 8) - 2U) >> 1;
#else
    return (reinterpret_cast< const u8* >(mMessage)[0] - 2U) >> 1;
#endif''');s=s.replace('return *reinterpret_cast< const u32* >(reinterpret_cast< const u8* >(mMessage) + index * 4 + 4);','''#if defined(TARGET_PC)
    // Parameters are original big-endian pairs of retained UTF-16 code units.
    return (static_cast<u32>(mMessage[2 + index * 2]) << 16) | static_cast<u32>(mMessage[3 + index * 2]);
#else
    return *reinterpret_cast< const u32* >(reinterpret_cast< const u8* >(mMessage) + index * 4 + 4);
#endif''');p.write_text(s)
p=root/'src/Game/NPC/TalkNodeCtrl.cpp';s=p.read_text().replace('MessageEditorMessageTag messageTag = MessageEditorMessageTag((const wchar_t*)&info._0[2]);\n\n    if (((char*)messageTag.mMessage)[1] != 8 || messageTag.mMessage[1] != 0) {','''#if defined(TARGET_PC)
    MessageEditorMessageTag messageTag(reinterpret_cast<const wchar_t*>(info._0) + 1);
    if (!messageTag.isGroupTagId(8, 0)) {
#else
    MessageEditorMessageTag messageTag = MessageEditorMessageTag((const wchar_t*)&info._0[2]);
    if (((char*)messageTag.mMessage)[1] != 8 || messageTag.mMessage[1] != 0) {
#endif''');p.write_text(s)
p=root/'src/compat/OriginalMessageLineQueries.cpp';s=p.read_text();a=s.index('MessageEditorMessageTag::MessageEditorMessageTag');b=s.index('bool MessageEditorMessageTag::isGroupTagId');s=s[:a]+s[b:];p.write_text(s)
p=root/'src/Game/Util/JMapUtil.hpp';s=p.read_text();i=s.index('    bool getJMapInfoMessageID(');end=s.index('\n',i);s=s[:end]+'''\n\n    inline s32 getMessageID(const JMapInfoIter& rIter) {
        s32 msgId;
        getJMapInfoMessageID(rIter, &msgId);
        return msgId;
    }
'''+s[end:];p.write_text(s)
