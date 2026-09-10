import os, subprocess, pathlib, time, json, selectors, hashlib, argparse, re
parser=argparse.ArgumentParser()
parser.add_argument("--name",required=True)
parser.add_argument("--frames",type=int,default=600)
parser.add_argument("--seconds",type=float,default=40)
parser.add_argument("--buttons",default="")
args=parser.parse_args()
root=pathlib.Path(__file__).resolve().parents[2]
note=pathlib.Path(__file__).resolve().parent
binary=root/"build/macosx/arm64/debug/smg-pc-showcase"
cmd=[str(binary),"gateway","--disc",str(root/"Super Mario Wii - Galaxy Adventure (Korea).rvz"),"--max-frames",str(args.frames)]
env={**os.environ,"SMGPC_DEBUG_SIMULATION_TIMING":"1"}
if args.buttons: env["SMGPC_DEBUG_WPAD_BUTTON_SCRIPT"]=args.buttons
start=time.monotonic(); frames=[]; data=b""; timed_out=False
with (note/(args.name+".log")).open("wb") as log:
 proc=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env)
 sel=selectors.DefaultSelector();sel.register(proc.stdout,selectors.EVENT_READ)
 while sel.get_map():
  if time.monotonic()-start>args.seconds and proc.poll() is None:
   timed_out=True;proc.kill()
  for key,_ in sel.select(0.2):
   chunk=os.read(key.fd,65536)
   if not chunk:sel.unregister(key.fileobj);break
   log.write(chunk);log.flush();data+=chunk
   while b"\n" in data:
    line,data=data.split(b"\n",1)
    if b"[smgpc:timing]" in line:
     match=re.search(rb"tick=(\d+) present=(\d+)",line)
     if match:frames.append({"wall_seconds":time.monotonic()-start,"tick":int(match[1]),"present":int(match[2])})
 code=proc.wait()
result={"command":cmd,"binary_sha256":hashlib.sha256(binary.read_bytes()).hexdigest(),"exit":code,"timeout":timed_out,"wall_seconds":time.monotonic()-start,"frames":frames,"buttons":args.buttons}
if len(frames)>1:
 result["present_fps"]=(len(frames)-1)/(frames[-1]["wall_seconds"]-frames[0]["wall_seconds"])
 result["simulation_ticks_per_second"]=(frames[-1]["tick"]-frames[0]["tick"])/(frames[-1]["wall_seconds"]-frames[0]["wall_seconds"])
(note/(args.name+".json")).write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps({k:v for k,v in result.items() if k!="frames"},indent=2))
