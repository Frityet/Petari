#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,re,struct,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-creator-20260903'
spec=importlib.util.spec_from_file_location('creator',N/'verify-original.py');v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
PREFIXES=('cCreateTable','cName2ArchiveNamesTable','cName2MakeArchiveListFuncTable','cPlayerArchiveLoaderObjTable')
class Elf(v.Elf):
 def references(self,n):
  rows=super().references(n)
  for x in rows:
   if x['symbol'].startswith(PREFIXES):x.pop('value_hex',None)
  return rows

def owners():
 result=[];source=None
 for line in (R/'config/RMGK01/splits.txt').read_text().splitlines():
  if line and not line[0].isspace() and line.endswith(':'):source=line[:-1]
  m=re.search(r'\.([\w]+)\s+start:(0x\w+) end:(0x\w+)',line)
  if m:result.append((int(m[2],16),int(m[3],16),source))
 return result

def main():
 src=R/'src/Game/NameObj/NameObjFactory.cpp';target=R/'build/original-collision-owner-20260903/retail/obj/Game/NameObj/NameObjFactory.o';obj=B/'NameObjFactory.o';cmd=v.d.o.c.compiler('cflags_game')+['-c',str(src),'-o',str(obj)];r=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(N/'factory-original-compile.log').write_text(r.stdout+r.stderr);assert r.returncode==0
 subprocess.run([R/'build/tools/objdiff-cli','diff','-1',target,'-2',obj,'-o',B/'factory-diff.json','--format','json-pretty'],check=True,stdout=subprocess.DEVNULL)
 a,b=Elf(target),Elf(obj);dol=v.d.o.c.DOL.read_bytes();tables=[]
 for p in PREFIXES:
  name=next(s[0] for s in a.symbols if s[0].startswith(p));record=v.compare_pointer_object(a,b,name,dol);(N/(p+'-evidence.json')).write_text(json.dumps(record,indent=2)+'\n');tables.append(record)
 j=json.loads((B/'factory-diff.json').read_text());methods=[]
 for left in j['left']['symbols']:
  name=left['name']
  if not name.startswith(('getCreator__14','requestMountObjectArchives__14','isReadResourceFromDVD__14','isPlayerArchiveLoaderObj__14','getName2CreateFunc__14','getMountObjectArchiveList__14')):continue
  refs=v.d.o.relocate(b,a,name,v.d.o.addresses[name],dol);methods.append(dict(symbol=name,retail_address=hex(v.d.o.addresses[name]),bytes=len(a.code(name)),match_percent=left['match_percent'],all_instructions_identical_after_verified_relocations=True,verified_relocations=refs))
 ranges=owners();undefined=[]
 # Direct undefined graph of the complete original compiler object, not the native subset.
 for n,at,size,sec in b.symbols:
  if sec!=0 or not n:continue
  address=v.d.o.addresses.get(n);owner=next((s for lo,hi,s in ranges if address is not None and lo<=address<hi),None)
  undefined.append(dict(symbol=n,retail_address=hex(address) if address else None,retail_provider=owner,root_source_exists=bool(owner and (R/'src'/owner).exists()),native_source_exists=bool(owner and (R/'pc-port/src'/owner).exists())))
 create=tables[0];raw=a.code(create['symbol']);entry_offsets={int(x['offset'],16):x['value'] for x in create['entries']};entries=[]
 for i in range(len(raw)//12):
  values=[entry_offsets.get(i*12+k) for k in (0,4,8)]
  entries.append(dict(index=i,name=values[0],creator=values[1],archive=values[2]))
 archive_callbacks=sorted(set(x['value'] for x in tables[2]['entries'] if int(x['offset'],16)%8==4))
 # Record actual header declarations that still encode console-sized opaque fields.
 padded=[]
 for header in sorted((R/'include/Game').rglob('*.hpp')):
  for i,line in enumerate(header.read_text().splitlines(),1):
   if re.search(r'\[.*0x[0-9A-Fa-f]+.*-\s*sizeof\(',line):padded.append(dict(header=str(header.relative_to(R)),line=i,declaration=line.strip()))
 summary=dict(source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),compiler_command=cmd,lookup_methods=methods,creator_rows=len(entries),unique_creator_functions=len({x['creator'] for x in entries if x['creator']}),original_null_creator_rows=[x for x in entries if not x['creator']],archive_rows=len(a.code(tables[1]['symbol']))//8,archive_callback_rows=len(a.code(tables[2]['symbol']))//8,unique_archive_callbacks=len(archive_callbacks),player_archive_names=len(a.code(tables[3]['symbol']))//4,table_pointer_count=sum(len(t['entries']) for t in tables),direct_undefined_symbols=len(undefined),direct_undefined_game_provider_count=len({x['retail_provider'] for x in undefined if x['retail_provider'] and x['retail_provider'].startswith('Game/')}),creator_entries=entries,archive_callbacks=archive_callbacks,direct_undefined_graph=undefined,console_size_opaque_header_declarations=padded)
 (N/'factory-audit.json').write_text(json.dumps(summary,indent=2)+'\n');print({k:val for k,val in summary.items() if isinstance(val,int)},'lookup bytes',sum(x['bytes'] for x in methods))
 # Current native object dependency inventory. Availability is a symbol fact only,
 # not proof that the present provider follows the retail body or owns its graph.
 nm='/opt/homebrew/opt/llvm/bin/llvm-nm';game=R/'pc-port/build/macosx/arm64/debug/libsmg-pc-game.a';common=game.with_name('libsmg-pc-common.a');render=game.with_name('libsmg-pc-render.a')
 libs=[game,common,render];defined=set()
 for lib in libs:
  result=subprocess.run([nm,'--defined-only','--just-symbol-name',str(lib)],capture_output=True,text=True,check=True);defined.update(result.stdout.splitlines())
 result=subprocess.run([nm,'--undefined-only','--just-symbol-name',str(B/'planet-native.o')],capture_output=True,text=True,check=True);raw=result.stdout.splitlines();demangled=subprocess.run([nm,'--undefined-only','--demangle','--just-symbol-name',str(B/'planet-native.o')],capture_output=True,text=True,check=True).stdout.splitlines();assert len(raw)==len(demangled)
 rows=[dict(symbol=x,demangled=y,present_in_native_game_common_render=x in defined) for x,y in zip(raw,demangled)]
 (N/'planet-native-dependencies.json').write_text(json.dumps(dict(libraries=[dict(path=str(p.relative_to(R)),size=p.stat().st_size,mtime_ns=p.stat().st_mtime_ns) for p in libs],dependencies=rows),indent=2)+'\n');print('native planet',len(rows),'undefined,',sum(not x['present_in_native_game_common_render'] for x in rows),'absent from three native archives')
if __name__=='__main__':main()
