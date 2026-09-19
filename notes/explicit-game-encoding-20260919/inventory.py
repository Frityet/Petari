from pathlib import Path
import re,json
# C++ source-token inventory for the one-time explicit encoding migration.
# Comments and raw-string contents are lexed, never treated as arbitrary regex matches.
pattern=re.compile(r'(?P<comment>//[^\n]*|/\*[\s\S]*?\*/)|(?P<raw>(?:u8|u|U|L)?R"(?P<delim>[^ ()\\\t\r\n]{0,16})\([\s\S]*?\)(?P=delim)")|(?P<string>(?:u8|u|U|L)?"(?:\\[\s\S]|[^"\\])*")|(?P<char>(?:u8|u|U|L)?\'(?:\\[\s\S]|[^\'\\])*\')|(?P<space>\s+)|(?P<other>[A-Za-z_][A-Za-z_0-9]*|.)')
def tokens(text):
 for m in pattern.finditer(text):
  kind=m.lastgroup
  if kind in ('comment','space'): continue
  yield dict(kind=kind,start=m.start(),end=m.end(),text=m.group())
def inventory(path):
 text=path.read_text()
 ts=list(tokens(text));rows=[]
 for i,t in enumerate(ts):
  if t['kind'] not in ('string','raw','char'): continue
  s=t['text']
  if not any(ord(c)>127 for c in s) and not re.search(r'\\[uUN]',s): continue
  start=text.rfind('\n',0,t['start'])+1;end=text.find('\n',t['end'])
  highesc=[m.group() for m in re.finditer(r'\\x[0-9a-fA-F]+',s) if int(m.group()[2:],16) >=128] if '\\x' in s else []
  highesc += [m.group() for m in re.finditer(r'\\[0-7]{1,3}',s) if int(m.group()[1:],8)>=128]
  rows.append(dict(path=str(path),line=text.count('\n',0,t['start'])+1,kind=t['kind'],literal=s,context=text[start:end if end>=0 else None],previous=ts[i-1]['text'] if i else '',next=ts[i+1]['text'] if i+1<len(ts) else '',high_numeric_escapes=highesc))
 return rows
if __name__=='__main__':
 rows=[]
 for p in sorted(Path('src/Game').rglob('*')):
  if p.suffix in ('.cpp','.hpp','.h','.inl'): rows+=inventory(p)
 Path('notes/explicit-game-encoding-20260919/literals.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n')
 print(json.dumps({'files':len({r['path'] for r in rows}),'literals':len(rows),'non_narrow':[r for r in rows if not r['literal'].startswith(('"','R"'))],'mixed_numeric':[r for r in rows if r['high_numeric_escapes']],'array_context':[r for r in rows if re.search(r'\[[^\]]*\]\s*=',r['context'])]},ensure_ascii=False,indent=2))
