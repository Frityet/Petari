#!/usr/bin/env python3
from pathlib import Path
import subprocess,json,time,shutil,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-game-execution-charset-20260903'
P=B/'isolated-xmake';(P/'Game').mkdir(parents=True,exist_ok=True)
# Copy tools so fingerprint invalidation can be checked without changing the
# reviewed proposal while the test is running.
T=P/'tools';T.mkdir(exist_ok=True)
for name in ['game_execution_charset.py','game_execution_charset.lua']:
 shutil.copyfile(N/'staged/tools'/name,T/name)
script=T/'game_execution_charset.py';lexer=B/'game-literal-lexer'
header=P/'Game/Literal.hpp';header.write_text('#pragma once\ninline const char* game_literal(){ return "共"; }\ninline const char* original_header_path(){ return __FILE__; }\n')
(P/'library.cpp').write_text('#include "Game/Literal.hpp"\nextern "C" const char* library_literal(){return game_literal();}\nextern "C" const char* library_path(){return original_header_path();}\n')
(P/'main.cpp').write_text('''extern "C" const char* library_literal();
extern "C" const char* library_path();
#include <cstdio>
#include <cstring>
int main(){const char* game=library_literal();const char* host="日本";std::printf("game=%02x%02x host=%02x header=%s\\n",(unsigned char)game[0],(unsigned char)game[1],(unsigned char)host[0],library_path());}
''')
(P/'xmake.lua').write_text('''set_project("isolated-game-charset-proof")
set_version("1.0.0")
set_languages("c++23")
set_symbols("debug")
set_policy("build.ccache", false)
toolchain("proof_llvm")
    set_kind("standalone")
    set_toolset("cxx", "/opt/homebrew/opt/llvm/bin/clang++")
    set_toolset("ld", "/opt/homebrew/opt/llvm/bin/clang++")
toolchain_end()
includes("tools/game_execution_charset.lua")
target("literal-lib")
    set_kind("static")
    set_toolchains("proof_llvm")
    add_rules("smgpc.game_execution_charset")
    set_values("game.charset.script", path.join(os.projectdir(), "tools/game_execution_charset.py"))
    set_values("game.charset.lexer", "'''+str(lexer)+'''")
    set_values("game.charset.roots", path.join(os.projectdir(), "Game"))
    add_files("library.cpp")
target("literal-proof")
    set_kind("binary")
    add_deps("literal-lib")
    set_toolchains("proof_llvm")
    add_rules("smgpc.game_execution_charset")
    set_values("game.charset.script", path.join(os.projectdir(), "tools/game_execution_charset.py"))
    set_values("game.charset.lexer", "'''+str(lexer)+'''")
    set_values("game.charset.roots", path.join(os.projectdir(), "Game"))
    add_files("main.cpp")
''')
rows=[]
def run(cmd,label):
 p=subprocess.run(cmd,cwd=P,capture_output=True,text=True)
 (B/(label+'.log')).write_text(p.stdout+p.stderr);rows.append({'command':cmd,'cwd':str(P),'returncode':p.returncode,'output':p.stdout+p.stderr})
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 return p.stdout
run(['xmake','f','-y','-m','debug'],'xmake-config')
run(['xmake','-y','-v','literal-proof'],'xmake-first')
exe=next(p for p in (P/'build').rglob('literal-proof') if p.is_file());obj=next((P/'build').rglob('main.cpp.o'))
first=run([str(exe)],'xmake-first-run');assert 'game=8ba4 host=e6' in first and 'header=./Game/Literal.hpp' in first
archive=next((P/'build').rglob('libliteral-lib.a'));archive_hash=hashlib.sha256(archive.read_bytes()).hexdigest()
stamp=obj.stat().st_mtime_ns
run(['xmake','-y','-v','literal-proof'],'xmake-cached');assert obj.stat().st_mtime_ns==stamp
# Header dependency names must remain original despite compilation via VFS.
header.write_text(header.read_text().replace('"共"','"属"'))
run(['xmake','-y','-v','literal-proof'],'xmake-header-change');assert obj.stat().st_mtime_ns!=stamp
updated=run([str(exe)],'xmake-header-run');assert 'game=91ae host=e6' in updated
assert hashlib.sha256(archive.read_bytes()).hexdigest()!=archive_hash
stamp=obj.stat().st_mtime_ns
script.write_text(script.read_text()+'\n# Test-only generator revision changes the compiler fingerprint.\n')
run(['xmake','-y','-v','literal-proof'],'xmake-tool-change');assert obj.stat().st_mtime_ns!=stamp
# The object carries original source/debug file paths, not content-addressed
# external compiler views. The mapped header __FILE__ check above is stricter
# than checking the main source, which is deliberately not mapped.
dwarf=run(['/opt/homebrew/opt/llvm/bin/llvm-dwarfdump','--debug-line',str(next((P/'build').rglob('library.cpp.o')))],'xmake-debug-lines')
assert 'Literal.hpp' in dwarf and '/views/' not in dwarf
(N/'build-hook-proof.json').write_text(json.dumps({'commands':rows,'first':first,'header_update':updated,'unchanged_object_reused':True,'header_change_recompiled':True,'tool_change_recompiled':True,'debug_paths_preserved':True,'static_archive_and_dependent_relinked':True},indent=2)+'\n')
print(first+updated+'isolated xmake cached/header/tool rebuild and source paths pass')
