from pathlib import Path
import json,re,hashlib,collections
root=Path.cwd(); notes=root/'notes/compat-whole-surface-20260925'
scope=json.loads((notes/'render-effects-scope.json').read_text())
# Full-text lexical scan: ignore comments/whitespace for source-ownership matches,
# retaining actual literal values. This is not a semantic equivalence proof.
def normalize(s):
 return re.sub(r'\s+','',re.sub(r'//[^\n]*|/\*[\s\S]*?\*/','',s))
def functions(s):
 # Qualified/free top-level J3D and JPA functions, including multi-line heads.
 pattern=re.compile(r'^[ \t]*((?:[\w:<>,*&]+\s+)*)([\w:~]+|operator[^\s(]+)\s*\(([^;{}]*?)\)\s*(?:const\s*)?(?:noexcept\s*)?(?::[^\n{]*)?\s*\{',re.M)
 out=[]
 for m in pattern.finditer(s):
  name=m.group(2)
  if name in ('if','for','while','switch','catch'):continue
  n=m.end(); depth=1;state='normal'
  while n<len(s) and depth:
   c=s[n]; d=s[n:n+2]
   if state=='normal':
    if d=='//': state='comment';n+=1
    elif d=='/*':state='block';n+=1
    elif c=='"':state='string'
    elif c=="'":state='char'
    elif c=='{':depth+=1
    elif c=='}':depth-=1
   elif state=='comment':
    if c=='\n':state='normal'
   elif state=='block':
    if d=='*/':state='normal';n+=1
   else:
    if c=='\\':n+=1
    elif c==('"' if state=='string' else "'"):state='normal'
   n+=1
  if depth==0:out.append({'symbol':name,'line':s[:m.start()].count('\n')+1,'end':s[:n].count('\n')+1,'body':s[m.start():n],'normalized':normalize(s[m.start():n])})
 return out
original={}
for p in (root/'decomp/src').rglob('*.cpp'):
 if '/JSystem/' not in str(p) and '/Game/Util/' not in str(p) and '/Game/Map/Light' not in str(p) and '/Game/Screen/' not in str(p) and '/Game/LiveActor/Shadow' not in str(p) and '/Game/Effect/' not in str(p) and p.name not in ('J3DModelX.cpp','Overwrite.cpp'):continue
 text=p.read_text(errors='replace')
 original[str(p.relative_to(root))]=(text,functions(text))
by_symbol=collections.defaultdict(list)
for p,(text,funcs) in original.items():
 for f in funcs:by_symbol[f['symbol']].append((p,f))
rows=[]
for entry in scope:
 p=root/entry['path'];s=p.read_text(); funcs=functions(s); fs=[]
 for f in funcs:
  candidates=by_symbol.get(f['symbol'],[])
  exact=[{'path':d,'line':other['line']} for d,other in candidates if f['normalized']==other['normalized']]
  fs.append({'symbol':f['symbol'],'line':f['line'],'end':f['end'],'exact_comment_whitespace_normalized':exact,'same_symbol_donors':[{'path':d,'line':other['line']} for d,other in candidates]})
 rows.append({'path':entry['path'],'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'snapshot_matches':hashlib.sha256(p.read_bytes()).hexdigest()==entry['sha256'],'line_count':len(s.splitlines()),'includes':re.findall(r'^#include [<"]([^>"\n]+)',s,re.M),'functions':fs})
(notes/'render-effects-source-scan.json').write_text(json.dumps(rows,indent=2)+'\n')
for r in rows:
 if '/J3D' in r['path']:
  fs=r['functions'];same=sum(bool(f['same_symbol_donors']) for f in fs);exact=sum(bool(f['exact_comment_whitespace_normalized']) for f in fs)
  print(Path(r['path']).name, 'functions',len(fs),'same-symbol-original',same,'exact',exact)
  print('nonexact',', '.join(f['symbol'] for f in fs if not f['exact_comment_whitespace_normalized']))
