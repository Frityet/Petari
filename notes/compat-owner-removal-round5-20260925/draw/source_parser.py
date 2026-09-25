import re
def normalize(s):
 return re.sub(r'\s+','',re.sub(r'//[^\n]*|/\*[\s\S]*?\*/','',s))
def functions(s):
 # Qualified/free top-level J3D and JPA functions, including multi-line heads.
 pattern=re.compile(r'^[ \t]*((?:[\w:<>,*&]+\s+)*)([\w:~=]+|operator[^\s(]+)\s*\(([^;{}]*?)\)\s*(?:const\s*)?(?:noexcept\s*)?(?::[^\n{]*)?\s*\{',re.M)
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

