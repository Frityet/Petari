from pathlib import Path
import re,json,hashlib
# Preserve comments, character/wide/raw literals and all other donor text. Only
# ordinary narrow Unicode string groups require the port's CP932 literal API.
TOKEN=re.compile(r'//[^\n]*(?:\\\n[^\n]*)*|/\*[\s\S]*?\*/|(?:u8|u|U|L)?R"(?P<delim>[^ ()\\\t\r\n]{0,16})\([\s\S]*?\)(?P=delim)"|(?:u8|u|U|L)?"(?:\\[\s\S]|[^"\\])*"|(?:u8|u|U|L)?\'(?:\\[\s\S]|[^\'\\])*\'')
TRIVIA=re.compile(r'(?:\s|//[^\n]*(?:\n|$)|/\*[\s\S]*?\*/)*\Z')
def encode(text):
 tokens=list(TOKEN.finditer(text));edits=[];i=0
 while i<len(tokens):
  first=tokens[i];i+=1
  if not first.group().startswith('"'):continue
  end=first.end();unicode=any(ord(c)>127 for c in first.group())
  while i<len(tokens) and tokens[i].group().startswith('"') and TRIVIA.fullmatch(text[end:tokens[i].start()]):
   unicode |= any(ord(c)>127 for c in tokens[i].group());end=tokens[i].end();i+=1
  if unicode:edits.append((first.start(),end))
 for a,b in reversed(edits):text=text[:a]+'CP932('+text[a:b]+')'+text[b:]
 if edits:text='#include "compat/Cp932Literal.hpp"\n'+text
 return text,len(edits)
entries=[]
for source in sorted(Path('decomp/src/Game').rglob('*')):
 if source.suffix not in ('.cpp','.c'):continue
 dest=Path('src')/source.relative_to('decomp/src')
 if dest.exists():continue
 raw=source.read_bytes();text,count=encode(raw.decode())
 dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(text)
 entries.append({'source':str(source),'destination':str(dest),'donor_sha256':hashlib.sha256(raw).hexdigest(),'cp932_literal_groups':count})
n=Path('notes/compat-owner-removal-round17-20260925')
(n/'missing-game-source-imports.json').write_text(json.dumps(entries,indent=2)+'\n')
print('Imported',len(entries),'missing Game source files; existing native paths unchanged')
