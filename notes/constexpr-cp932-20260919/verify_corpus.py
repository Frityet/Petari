"""Independent lexical extraction and compiler-byte proof for migrated literals."""
import argparse, hashlib, json, re, struct, subprocess, tempfile
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('--cxx', default='clang++')
args=parser.parse_args()
root=Path.cwd()
notes=root/'notes/constexpr-cp932-20260919'
before=root/'notes/explicit-game-encoding-20260919/before'
manifest=json.loads((before.parent/'migration.json').read_text())
# Whitespace/comments separate tokens but do not break adjacent literal runs.
lexer=re.compile(r'(?P<comment>//[^\n]*|/\*.*?\*/)|(?P<raw>R"(?P<delim>[^\s()\\]{0,16})\(.*?\)(?P=delim)")|(?P<string>"(?:\\.|[^"\\])*")|(?P<char>\'(?:\\.|[^\'\\])*\')|(?P<other>[^\s])', re.DOTALL)
expressions=[]
contexts=[]
for row in manifest:
    source=(before/row['path']).read_text()
    tokens=[m for m in lexer.finditer(source) if m.lastgroup!='comment']
    index=0
    current=[]
    while index<len(tokens):
        if tokens[index].lastgroup not in ('string','raw'):
            index+=1; continue
        first=index
        while index<len(tokens) and tokens[index].lastgroup in ('string','raw'):
            index+=1
        start,end=tokens[first].start(),tokens[index-1].end()
        expression=source[start:end]
        if not (any(ord(c)>127 for c in expression) or re.search(r'\\[uUN]',expression)):
            continue
        # All affected literal runs must be narrow and token-preserving.
        if first and tokens[first-1].end()==start:
            assert tokens[first-1].group() not in ('L','u','U'), (row['path'],expression)
        current.append((expression,row['path'],source.count('\n',0,start)+1))
        previous=source[max(0,source.rfind(';',0,start)+1):start]
        if re.search(r'\bchar\s+\w+\s*\[[^\]]*\]\s*=\s*$',previous):
            contexts.append({'path':row['path'],'line':source.count('\n',0,start)+1,'kind':'char-array-initialization'})
    assert len(current)==row['wrappers'], (row['path'],len(current),row['wrappers'])
    expressions.extend(current)
assert len(expressions)==3580

with tempfile.TemporaryDirectory(prefix='petari-cp932-corpus-') as temporary:
    temp=Path(temporary)
    cpp=temp/'corpus.cpp'
    header='''#include "compat/Cp932Literal.hpp"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
template<std::size_t N, std::size_t M> void dump(const char (&source)[N], const char (&encoded)[M]) {
    const std::uint32_t sizes[] = {N,M};
    if (std::fwrite(sizes,sizeof(sizes),1,stdout)!=1 || std::fwrite(source,1,N,stdout)!=N || std::fwrite(encoded,1,M,stdout)!=M) std::abort();
}
int main() {
'''
    cpp.write_text(header+''.join('dump('+text+', CP932('+text+'));\n' for text,_,_ in expressions)+'}\n')
    generated_hash=hashlib.sha256(cpp.read_bytes()).hexdigest()
    command=[args.cxx,'-std=c++23','-I',str(root/'src'),str(cpp),'-o',str(temp/'corpus')]
    build=subprocess.run(command,capture_output=True,text=True)
    (notes/'corpus-compile.log').write_text(build.stdout+build.stderr)
    assert build.returncode==0, build.stderr
    data=subprocess.check_output([str(temp/'corpus')])
    cursor=0; source_bytes=0; encoded_bytes=0; nul_literals=0
    for expression,path,line in expressions:
        n,m=struct.unpack_from('=II',data,cursor);cursor+=8
        source=data[cursor:cursor+n];cursor+=n
        encoded=data[cursor:cursor+m];cursor+=m
        expected=source.decode('utf-8').encode('cp932')
        assert encoded==expected, (path,line,expression,encoded,expected)
        assert source[-1:]==encoded[-1:]==b'\0'
        nul_literals+=source[:-1].count(b'\0')>0
        source_bytes+=n; encoded_bytes+=m
    assert cursor==len(data)
    report={'status':'passed','files':len(manifest),'literal_runs':len(expressions),
            'source_bytes_including_terminators':source_bytes,'encoded_bytes_including_terminators':encoded_bytes,
            'embedded_nul_runs':nul_literals,'array_initializer_hazards':contexts,
            'generated_translation_unit_sha256':generated_hash,'framed_compiler_output_sha256':hashlib.sha256(data).hexdigest(),
            'compiler':args.cxx,'cpp_standard':'c++23'}
    (notes/'corpus-results.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
