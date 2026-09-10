from pathlib import Path
import hashlib,json,os,subprocess,time
ROOT=Path(__file__).resolve().parents[3]
N=Path(__file__).resolve().parent
C='/opt/homebrew/opt/llvm/bin/clang++'
P=ROOT/'script/game_execution_charset.py'
H=N/'game-literal-preprocessor'
F=N/'fixture'
T=N/'temporary';T.mkdir(exist_ok=True)
ENV=dict(os.environ,TMPDIR=str(T))
results=[]
def run(cmd,label,want=0):
 start=time.monotonic();r=subprocess.run([str(x) for x in cmd],cwd=ROOT,env=ENV,text=True,capture_output=True)
 (N/(label+'.log')).write_text(r.stdout+r.stderr)
 record={'label':label,'command':[str(x) for x in cmd],'exit':r.returncode,'seconds':time.monotonic()-start}
 results.append(record)
 assert (r.returncode==0)==(want==0),(label,r.returncode,r.stdout,r.stderr)
 assert not list(T.iterdir()),(label,'temporary compiler output leaked')
 return r

def compile(source,label,extra=(),want=0,roots=None):
 src=F/source;obj=N/(label+'.o')
 admitted=['--game-root',F/'Game'] if roots is None else roots
 cmd=['python3',P,'--compiler',C,'--helper',H,*admitted,'--report',N/(label+'.json'),'--','-std=c++23','-O2','-g','-MMD','-MF',N/(label+'.d'),*extra,'-c',src,'-o',obj]
 run(cmd,label,want)
 return obj

before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [ROOT/'src/Game/System/GameEventFlagTable.cpp',F/'Game/Literals.hpp',F/'main.cpp']}
obj=compile('main.cpp','fixture-final')
run([C,obj,'-o',N/'fixture-final'],'fixture-final-link')
run([N/'fixture-final'],'fixture-final-runtime')
dep=(N/'fixture-final.d').read_text()
assert str(F/'Game/Literals.hpp') in dep and 'smgpc-game-charset' not in dep
dwarf=run(['/opt/homebrew/opt/llvm/bin/llvm-dwarfdump','--debug-info',obj],'fixture-dwarf').stdout
assert str(F/'main.cpp') in dwarf and 'smgpc-game-charset' not in dwarf

(F/'Game/Bytes.cpp').write_text(r'''#include <cstring>
#define WIDE(x) u##x
#define PART "共"
const char *raw = R"tag(表\u5171
共)tag";
static_assert(sizeof("共") == 3);
static_assert('共' == 0x8ba4);
static_assert(sizeof(u8"共") == 4);
static_assert(sizeof(u"日" PART) == 6);
static_assert(WIDE("共")[0] == 0x5171);
int main() {
 return std::strcmp("\u5171", "\213\244") ||
        std::strcmp("\u{5171}", "\213\244") ||
        std::strcmp("\N{CJK UNIFIED IDEOGRAPH-5171}", "\213\244") ||
        std::strcmp(raw, "\225\134\\u5171\n\213\244");
}
''')
obj=compile('Game/Bytes.cpp','bytes')
run([C,obj,'-o',N/'bytes'],'bytes-link');run([N/'bytes'],'bytes-runtime')

(F/'Host.cpp').write_text('static_assert(sizeof("共") == 4);\nconst char* host = "共";\n')
compile('Host.cpp','host')
assert json.loads((N/'host.json').read_text())['converted_literals']==0
(F/'OriginalProvider.cpp').write_text('static_assert(sizeof("共") == 3);\nconst char* game = "共";\n')
compile('OriginalProvider.cpp','explicit-provider',roots=['--game-file',F/'OriginalProvider.cpp'])

(F/'Game/Invalid.cpp').write_text('const char *unsupported = "😀";\n')
r=compile('Game/Invalid.cpp','invalid-codepoint',want=1)
assert 'not representable' in (N/'invalid-codepoint.log').read_text()
(F/'Game/Utf8.cpp').write_text('const char8_t *valid = u8"😀";\n')
compile('Game/Utf8.cpp','explicit-utf8')
(F/'Mixed.cpp').write_text('#include "Game/Literals.hpp"\nconst char *mixed = GAME_NARROW "日本";\n')
compile('Mixed.cpp','mixed-domains',want=1)
assert 'mix Game and host' in (N/'mixed-domains.log').read_text()
(F/'Game/Diagnostic.cpp').write_text('const char *text = "共";\nint broken = no_such_identifier;\n')
compile('Game/Diagnostic.cpp','diagnostic',want=1)
assert 'Game/Diagnostic.cpp:2:' in (N/'diagnostic.log').read_text()

(F/'Game/Conditional.cpp').write_text("#if '共' == 0x8ba4\nint branch = 1;\n#else\nint branch = 0;\n#endif\n")
compile('Game/Conditional.cpp','conditional-character',want=1)
assert 'before preprocessor evaluation' in (N/'conditional-character.log').read_text()
(F/'Game/ConditionalMacro.cpp').write_text("#define ORIGINAL_CHAR '共'\n#if ORIGINAL_CHAR == 0x8ba4\nint branch = 1;\n#endif\n")
compile('Game/ConditionalMacro.cpp','conditional-macro-character',want=1)
assert 'before preprocessor evaluation' in (N/'conditional-macro-character.log').read_text()

# The rule delegates link and compiler-probe invocations without preprocessing.
run(['python3',P,'--compiler',C,'--helper',H,'--game-root',F/'Game','--','--version'],'version-delegation')
for p,digest in before.items():assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==digest
(N/'wrapper-proofs.json').write_text(json.dumps({'results':results,'source_hashes':before,'temporary_files_remaining':len(list(T.iterdir()))},indent=2)+'\n')
print(json.dumps({'checks':len(results),'all_passed':True,'temporary_files_remaining':0}))
