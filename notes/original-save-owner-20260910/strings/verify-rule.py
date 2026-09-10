from pathlib import Path
import subprocess,json,time,shutil,hashlib,os
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;P=N/'isolated-xmake'
(P/'src/Game').mkdir(parents=True,exist_ok=True);(P/'script').mkdir(exist_ok=True)
for name in ['game_execution_charset.py','game_literal_preprocessor.cpp','game_execution_charset.lua']:
 shutil.copyfile(R/'script'/name,P/'script'/name)
header=P/'src/Game/Literal.hpp';header.write_text('#pragma once\ninline const char* game_literal(){return "共";}\ninline const char* game_path(){return __FILE__;}\n')
(P/'library.cpp').write_text('#include "src/Game/Literal.hpp"\nextern "C" const char* original_name(){return game_literal();}\nextern "C" const char* original_path(){return game_path();}\n')
(P/'sdk.cpp').write_text('static_assert(sizeof("共") == 4);\nextern "C" const char* sdk_name(){return "日本";}\n')
(P/'main.cpp').write_text('''#include <cstdio>
extern "C" const char *original_name();
extern "C" const char *original_path();
extern "C" const char *sdk_name();
int main(){auto p=original_name();std::printf("game=%02x%02x host=%02x sdk=%02x path=%s\\n",(unsigned char)p[0],(unsigned char)p[1],(unsigned char)"日本"[0],(unsigned char)sdk_name()[0],original_path());}
''')
(P/'xmake.lua').write_text('''set_project("isolated-game-charset-rule")
set_languages("c++23")
set_symbols("debug")
set_policy("build.ccache", true)
toolchain("proof_llvm")
    set_kind("standalone")
    set_toolset("cxx", "/opt/homebrew/opt/llvm/bin/clang++")
    set_toolset("ld", "/opt/homebrew/opt/llvm/bin/clang++")
    set_toolset("ar", "/opt/homebrew/opt/llvm/bin/llvm-ar")
toolchain_end()
set_toolchains("proof_llvm")
includes("script/game_execution_charset.lua")
add_rules("smgpc.game_execution_charset")
target("aurora-exclusion-proof")
    set_kind("static")
    add_files("sdk.cpp")
    after_config(function(target)
        assert(not target:tool("cxx"):find("game_execution_charset",1,true))
        assert(target:policy("build.ccache") == true)
    end)
target("smg-pc-literal-lib")
    set_kind("static")
    add_files("library.cpp")
    after_config(function(target)
        assert(target:tool("cxx"):find("game_execution_charset",1,true))
        assert(target:policy("build.ccache") == false)
    end)
target("smg-pc-literal-proof")
    set_kind("binary")
    add_files("main.cpp")
    add_deps("smg-pc-literal-lib", "aurora-exclusion-proof")
''')
rows=[]
def run(cmd,label):
 if cmd[0]=='xmake':
  at=2 if len(cmd)>1 and cmd[1]=='f' else 1
  cmd=cmd[:at]+['-P',str(P)]+cmd[at:]
 start=time.monotonic();p=subprocess.run([str(x) for x in cmd],cwd=P,capture_output=True,text=True)
 (N/(label+'.log')).write_text(p.stdout+p.stderr)
 rows.append({'label':label,'command':[str(x) for x in cmd],'cwd':str(P),'exit':p.returncode,'seconds':time.monotonic()-start})
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 return p.stdout
run(['xmake','f','-y','-m','debug'],'rule-config')
run(['xmake','-y','-v','smg-pc-literal-proof'],'rule-first')
exe=next(p for p in (P/'build').rglob('smg-pc-literal-proof') if p.is_file())
obj=next((P/'build').rglob('library.cpp.o'));main=next((P/'build').rglob('main.cpp.o'));sdk=next((P/'build').rglob('sdk.cpp.o'))
archive=next((P/'build').rglob('libsmg-pc-literal-lib.a'))
first=run([exe],'rule-first-runtime');assert 'game=8ba4 host=e6 sdk=e6' in first
assert 'src/Game/Literal.hpp' in first and 'smgpc-game-charset' not in first
stamps={str(p):p.stat().st_mtime_ns for p in [obj,main,sdk,archive,exe]}
run(['xmake','-y','-v','smg-pc-literal-proof'],'rule-no-op')
assert all(Path(p).stat().st_mtime_ns==stamp for p,stamp in stamps.items())
time.sleep(1.1)
header.write_text(header.read_text().replace('共','属'))
run(['xmake','-y','-v','smg-pc-literal-proof'],'rule-header-change')
second=run([exe],'rule-header-runtime');assert 'game=91ae host=e6 sdk=e6' in second
assert obj.stat().st_mtime_ns != stamps[str(obj)] and sdk.stat().st_mtime_ns == stamps[str(sdk)]
assert archive.stat().st_mtime_ns != stamps[str(archive)] and exe.stat().st_mtime_ns != stamps[str(exe)]
for suffix in ['.py','.cpp','.lua']:
 tool=P/'script'/({'py':'game_execution_charset.py','cpp':'game_literal_preprocessor.cpp','lua':'game_execution_charset.lua'}[suffix[1:]])
 before={str(p):p.stat().st_mtime_ns for p in [obj,main,sdk,archive,exe]}
 old=tool.stat();comment='#' if suffix=='.py' else ('//' if suffix=='.cpp' else '--')
 tool.write_text(tool.read_text()+'\n'+comment+' Isolated tool fingerprint mutation.\n')
 os.utime(tool,ns=(old.st_atime_ns,old.st_mtime_ns))
 run(['xmake','-y','-v','smg-pc-literal-proof'],'rule-tool-change-'+suffix[1:])
 assert all(Path(p).stat().st_mtime_ns!=stamp for p,stamp in before.items() if p!=str(sdk))
 assert sdk.stat().st_mtime_ns==before[str(sdk)]
run([exe],'rule-final-runtime')
dwarf=run(['/opt/homebrew/opt/llvm/bin/llvm-dwarfdump','--debug-line',obj],'rule-debug-lines')
assert 'Literal.hpp' in dwarf and 'smgpc-game-charset' not in dwarf
(N/'rule-proofs.json').write_text(json.dumps({'commands':rows,'initial_output':first,'changed_header_output':second,'no_op_reuses_objects_archive_and_binary':True,'header_dependency_rebuilds_and_relinks':True,'same_mtime_python_cpp_lua_edits_rebuild':True,'aurora_compiler_cache_and_objects_unchanged':True,'source_paths_preserved':True},indent=2)+'\n')
print('PASS: isolated rule cache, header dependencies, Python/C++/Lua fingerprints, source paths, and Aurora exclusion')
