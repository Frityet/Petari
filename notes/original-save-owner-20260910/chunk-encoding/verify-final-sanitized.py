"""Build only isolated current objects; never run Xmake or reuse root archives."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import hashlib,json,os,subprocess,time

ROOT=Path(__file__).resolve().parents[3]
NOTE=Path(__file__).resolve().parent
STRINGS=NOTE.parent/'strings'
WRAPPER=ROOT/'script/game_execution_charset.py'
HELPER=STRINGS/'game-literal-preprocessor'
# Retain the earlier exact native compiler include/ABI flags as the reproducible
# command template. All source files are re-read and freshly compiled below.
previous=json.loads((NOTE/'fixture-compile.json').read_text())
compiler=previous[0]['command'][0]
llvmconfig=Path(compiler).with_name('llvm-config')
includedir=subprocess.check_output([str(llvmconfig),'--includedir'],text=True).strip()
libdir=subprocess.check_output([str(llvmconfig),'--libdir'],text=True).strip()
helper_command=[compiler,'-std=c++23','-O2',str(ROOT/'script/game_literal_preprocessor.cpp'),
                '-I'+includedir,'-L'+libdir,'-Wl,-rpath,'+libdir,'-lclang-cpp','-lLLVM','-o',str(HELPER)]
helper_result=subprocess.run(helper_command,cwd=ROOT,text=True,capture_output=True)
(NOTE/'final-sanitized-helper.log').write_text(helper_result.stdout+helper_result.stderr)
(NOTE/'final-sanitized-helper.json').write_text(json.dumps({'command':helper_command,'exit':helper_result.returncode},indent=2)+'\n')
if helper_result.returncode:raise SystemExit(helper_result.returncode)
commands=[]

def compile_one(row):
 source=ROOT/row['source']
 digest=hashlib.sha256(source.read_bytes()).hexdigest()
 obj=NOTE/(source.stem+'.final-sanitized.o')
 original=list(row['command'])
 compiler=original.pop(0)
 original[original.index('-o')+1]=str(obj)
 original[-1]=str(source)
 cmd=['python3',str(WRAPPER),'--compiler',compiler,'--helper',str(HELPER),
      '--game-root',str(ROOT/'src/Game'),'--report',str(NOTE/(source.stem+'.final-wrapper.json')),
      '--',*original]
 started=time.monotonic()
 result=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True)
 (NOTE/(source.stem+'.final-compile.log')).write_text(result.stdout+result.stderr)
 assert hashlib.sha256(source.read_bytes()).hexdigest()==digest, 'source changed during isolated compilation'
 return {'source':row['source'],'sha256':digest,
         'object':str(obj),'command':cmd,'exit':result.returncode,'seconds':time.monotonic()-started}

with ThreadPoolExecutor(max_workers=2) as pool:
 commands=list(pool.map(compile_one,previous))
(NOTE/'final-sanitized-compile.json').write_text(json.dumps(commands,indent=2)+'\n')
failed=[row for row in commands if row['exit']]
if failed:
 print(json.dumps(failed,indent=2));raise SystemExit(1)

link=json.loads((NOTE/'fixture-link.json').read_text())['command']
link=[next((row['object'] for old,row in zip(previous,commands) if argument==old['object']),argument) for argument in link]
exe=NOTE/'chunk-encoding-final-sanitized'
link[link.index('-o')+1]=str(exe)
result=subprocess.run(link,cwd=ROOT,text=True,capture_output=True)
(NOTE/'final-sanitized-link.log').write_text(result.stdout+result.stderr)
(NOTE/'final-sanitized-link.json').write_text(json.dumps({'command':link,'exit':result.returncode},indent=2)+'\n')
if result.returncode:print(result.stdout+result.stderr);raise SystemExit(result.returncode)

environment=dict(os.environ,ASAN_OPTIONS='halt_on_error=1:detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
started=time.monotonic();result=subprocess.run([str(exe)],cwd=ROOT,env=environment,text=True,capture_output=True)
(NOTE/'final-sanitized-run.log').write_text(result.stdout+result.stderr)
proof={'command':[str(exe)],'exit':result.returncode,'seconds':time.monotonic()-started,
       'environment':{key:environment[key] for key in ['ASAN_OPTIONS','UBSAN_OPTIONS']},
       'binary_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),
       'tool_sha256':{str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in [WRAPPER,ROOT/'script/game_literal_preprocessor.cpp',HELPER]},
       'output':result.stdout+result.stderr}
(NOTE/'final-sanitized-run.json').write_text(json.dumps(proof,indent=2)+'\n')
print(json.dumps(proof,indent=2));raise SystemExit(result.returncode)
