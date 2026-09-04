#!/usr/bin/env python3
from pathlib import Path
import importlib.util,sys,subprocess,json,hashlib,os,time
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-game-execution-charset-20260903'
TOOL=N/'staged/tools/game_execution_charset.py';LEXER=B/'game-literal-lexer';CLANG='/opt/homebrew/opt/llvm/bin/clang++'
spec=importlib.util.spec_from_file_location('charset',TOOL);charset=importlib.util.module_from_spec(spec);spec.loader.exec_module(charset)
B.mkdir(parents=True,exist_ok=True);commands=[]
def run(command,name,cwd=R):
 p=subprocess.run([str(x) for x in command],cwd=cwd,capture_output=True,text=True)
 (B/(name+'.log')).write_text(p.stdout+p.stderr);commands.append({'command':[str(x) for x in command],'returncode':p.returncode})
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 return p.stdout
run([CLANG,'-std=c++23',N/'staged/tools/game_literal_lexer.cpp','-I/opt/homebrew/opt/llvm/include','-L/opt/homebrew/opt/llvm/lib','-Wl,-rpath,/opt/homebrew/opt/llvm/lib','-lclang-cpp','-lLLVM','-o',LEXER],'lexer-build')
def tokens(path):return json.loads(run([LEXER,path],'lex-probe'))[0]['tokens']
def transform(text):
 path=B/'token-probe.cpp';path.write_bytes(text.encode());return charset.transform(text.encode(),tokens(path))[0].decode()
# Literal boundaries are owned by Clang. Mixed prefixed concatenation inherits
# the wider/Unicode kind; comments and fake quote characters cannot confuse it.
samples=[
 ('"共通常"','"'+charset.octal('共通常'.encode('cp932'))+'"'),
 ('\'共\'','\''+charset.octal('共'.encode('cp932'))+'\''),
 ('"\\u5171\\U0000901a常"','"'+charset.octal('共通常'.encode('cp932'))+'"'),
 ('"\\u{5171}\\N{CJK UNIFIED IDEOGRAPH-901A}"','"'+charset.octal('共通'.encode('cp932'))+'"'),
 ('"\\\\u5171 共"','"\\\\u5171 '+charset.octal('共'.encode('cp932'))+'"'),
 ('"\\xA1共A\\123"','"\\xA1'+charset.octal('共'.encode('cp932'))+'A\\123"'),
 ('R"tag(共\\u5171\\n"/*x*/)tag"','"'+charset.octal('共\\u5171\\n"/*x*/'.encode('cp932'))+'"'),
 ('"共\\\n通"','"'+charset.octal('共'.encode('cp932'))+'\\\n'+charset.octal('通'.encode('cp932'))+'"'),
 ('"\\u51\\\n71"','"'+charset.octal('共'.encode('cp932'))+'\\\n"'),
 ('R\\\n"tag(共)tag"','"\\\n'+charset.octal('共'.encode('cp932'))+'"'),
 ('R"(共\n通)"','"'+charset.octal('共\n'.encode('cp932'))+'\\\n'+charset.octal('通'.encode('cp932'))+'"'),
 ('R"(共\r\n通)"','"'+charset.octal('共\n'.encode('cp932'))+'\\\r\n'+charset.octal('通'.encode('cp932'))+'"'),
 ('R"(共\r通)"','"'+charset.octal('共\n'.encode('cp932'))+'\\\r'+charset.octal('通'.encode('cp932'))+'"'),
 ('u8"日本" u"日本" U"日本" L"日本"',None),
 ('u8\'共\' u\'共\' U\'共\' L\'共\'',None),
 ('L"日" /* "共" */ "本"',None),
 ('"日" u"本"',None),
 ('// "共"\n/* \'共\' */ const int 日本=0;',None),
 ('#include "日本.hpp"\n#line 8 "日本.cpp"\n',None),
 ('#inc\\\nlude "日本.hpp"\n',None),
]
for original,expected in samples:
 actual=transform(original)
 assert actual==(original if expected is None else expected),(original,actual,expected)
for bad in ['"😀"','"\\U00110000"','"\\uD800"']:
 try:transform(bad)
 except charset.EncodingError:pass
 else:raise AssertionError('unsupported/invalid narrow codepoint accepted '+bad)
assert transform('u8"😀"')=='u8"😀"'
# Actual root declarations stay unmodified while their compiler view contains
# original CP932 bytes. Existing Mario predicates are copied verbatim only to
# make the independent fixture link without the rest of Mario's vtable.
root=(R/'src/Game/Player/MarioEffect.cpp').read_text();bodies=[]
for name in ['isCommonEffect','isMaterialEffect']:
 start=root.index('bool MarioActor::'+name+'(');brace=root.index('{',start);end=brace+1;depth=1
 while depth:depth+=(root[end]=='{')-(root[end]=='}');end+=1
 bodies.append(root[start:end])
legacy=B/'original-literals.cpp'
legacy.write_text('typedef unsigned char u8;\nclass MarioActor { public: bool isCommonEffect(const char*) const; bool isMaterialEffect(const char*) const; };\n'+'\n'.join(bodies)+'''\nextern "C" {
extern const char plain[] = "共通常";
extern const char material[] = "属性着地";
extern const char trailing[] = "表ソ十";
extern const char joined[] = "共" /* preserve */ "通";
extern const char ascii_escape[] = "A\\t共\\n\\0X";
extern const int character = '共';
}\n''')
modern=B/'modern-literals.hpp'
modern.write_text('''#pragma once
// Unmodified UTF-8 comment: 共 "fake"
const char* ordinary_ucn = "\\u5171\\U0000901a";
const char* raw = R"tag(共\\u5171
通)tag";
const char* escaped_newline = "共\\
通";
const char8_t* utf8 = u8"日本😀";
const char16_t* utf16 = u"日" /* comment */ "本";
const char32_t* utf32 = U"日本";
const wchar_t* wide = L"日本";
constexpr unsigned long operator ""_size(const char*, unsigned long n) { return n; }
constexpr auto raw_size = R"x(共)x"_size;
''')
main=B/'main.cpp';main.write_text('''#include <cstdio>
#include <cstring>
#include <cassert>
#include "original-literals.cpp"
#include "modern-literals.hpp"
static void print(const char* name,const char* text,unsigned long count){std::printf("%s=",name);for(unsigned long i=0;i<count;++i)std::printf("%02x",static_cast<unsigned char>(text[i]));std::puts("");}
int main(){
print("plain",plain,sizeof(plain)); print("material",material,sizeof(material)); print("trailing",trailing,sizeof(trailing)); print("joined",joined,sizeof(joined)); print("ascii_escape",ascii_escape,sizeof(ascii_escape));
std::printf("character=%08x\\n",character);
assert(MarioActor().isCommonEffect(plain)); assert(!MarioActor().isMaterialEffect(plain)); assert(MarioActor().isMaterialEffect(material)); assert(!MarioActor().isCommonEffect(material));
print("ordinary_ucn",ordinary_ucn,std::strlen(ordinary_ucn));print("raw",raw,std::strlen(raw));print("escaped_newline",escaped_newline,std::strlen(escaped_newline));
assert(utf8[0]==char8_t(0xe6)&&utf8[6]==char8_t(0xf0));assert(utf16[0]==0x65e5&&utf16[1]==0x672c&&utf16[2]==0);assert(utf32[0]==0x65e5&&wide[1]==0x672c);assert(raw_size==2);
std::printf("file=%s\\n",__FILE__);std::puts("original_mario_prefixes=pass unicode_kinds=pass");
}
''')
source_hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [legacy,modern,main]}
report=charset.build(LEXER,[legacy,modern],B/'fixture-view')
overlay=B/'fixture-view/overlay.json'
run([CLANG,'-std=c++23','-g','-O0','-ivfsoverlay',overlay,'-MMD','-MF',B/'fixture.d',main,'-o',B/'fixture'],'native-fixture-build')
output=run([B/'fixture'],'native-fixture');print(output)
actual=dict(line.split('=',1) for line in output.splitlines() if '=' in line)
assert actual['ordinary_ucn']=='共通'.encode('cp932').hex()
assert actual['raw']=='共\\u5171\n通'.encode('cp932').hex()
assert actual['escaped_newline']=='共通'.encode('cp932').hex()
assert actual['file']==str(main)
assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==digest for p,digest in source_hashes.items())
deps=(B/'fixture.d').read_text();assert str(legacy) in deps and str(modern) in deps and '/views/' not in deps
# Header-only changes invalidate the view without changing original identity;
# repeated unchanged builds do not rewrite overlay/views and hit every cache.
mtime=overlay.stat().st_mtime_ns
cached=charset.build(LEXER,[legacy,modern],B/'fixture-view');assert cached['cache_hits']==2 and cached['lexed']==0 and overlay.stat().st_mtime_ns==mtime
old=modern.read_text();modern.write_text(old.replace('ordinary_ucn = "\\u5171\\U0000901a"','ordinary_ucn = "属性"'))
updated=charset.build(LEXER,[legacy,modern],B/'fixture-view');assert updated['cache_hits']==1 and updated['lexed']==1
run([CLANG,'-std=c++23','-ivfsoverlay',overlay,main,'-o',B/'fixture-updated'],'updated-build')
changed=run([B/'fixture-updated'],'updated-run');assert 'ordinary_ucn='+ '属性'.encode('cp932').hex() in changed
modern.write_text(old);charset.build(LEXER,[legacy,modern],B/'fixture-view')
cache=json.loads((B/'fixture-view/cache.json').read_text())
view_path=Path(cache['entries'][str(modern)]['view']);expected_view=view_path.read_bytes();view_path.write_bytes(b'// damaged cached view\n')
repaired=charset.build(LEXER,[legacy,modern],B/'fixture-view');assert repaired['lexed']==1 and view_path.read_bytes()==expected_view
bad=B/'diagnostic.cpp';bad.write_text('const char* raw=R"(共\n通)";\nstatic_assert(false, "failure");\n')
charset.build(LEXER,[bad],B/'diagnostic-view')
command=[CLANG,'-std=c++23','-fsyntax-only','-ivfsoverlay',str(B/'diagnostic-view/overlay.json'),str(bad)]
result=subprocess.run(command,cwd=R,capture_output=True,text=True)
commands.append({'command':command,'returncode':result.returncode,'expected_failure':True})
assert result.returncode!=0 and str(bad)+':3:' in result.stderr and '/views/' not in result.stderr
(N/'diagnostic-path.log').write_text(result.stderr)

# Explicit original-extract ranges leave host UI literals unchanged. A changed
# source identity or range is rejected before generating any compiler view.
compat=B/'mixed-compat.cpp';original_range='const char* game="共";'
original=B/'range-original.cpp';original.write_text(original_range)
compat.write_text(original_range+'\nconst char* host=u8"日本";\nconst char* host_narrow="日本";\n')
record={'path':str(compat),'sha256':charset.sha(compat.read_bytes()),'ranges':[{'begin':0,'end':len(original_range.encode()),'source':str(original),'source_begin':0,'source_end':len(original_range.encode())}]}
charset.build(LEXER,[],B/'extract-view',[record]);cache=json.loads((B/'extract-view/cache.json').read_text());view=Path(cache['entries'][str(compat)]['view']).read_text()
assert 'game="\\213\\244"' in view and 'host_narrow="日本"' in view
compat.write_text(compat.read_text()+'// changed\n')
try:charset.build(LEXER,[],B/'extract-view',[record])
except charset.EncodingError:pass
else:raise AssertionError('stale extraction provenance accepted')
# Full source inventory, including source-only originals not yet active native.
paths=sorted({p for directory in ['src/Game','include/Game','pc-port/src/Game'] for p in (R/directory).rglob('*') if p.suffix in {'.cpp','.hpp','.h','.inc'}})
full=charset.build(LEXER,paths,B/'all-game');print(json.dumps(full,indent=2))
(N/'native-proof.json').write_text(json.dumps({'commands':commands,'lexical_cases':len(samples),'rejection_cases':3,'source_unchanged':source_hashes,'native_output':output,'fixture':report,'cache':cached,'header_update':updated,'full_game':full},indent=2)+'\n')
(N/'native-fixture.log').write_text(output)
print('lexical semantics, actual prefix predicates, VFS identity, Unicode kinds, cache and provenance checks pass')
