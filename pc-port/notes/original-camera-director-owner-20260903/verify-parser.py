from pathlib import Path
import json, subprocess, os, hashlib
R=Path(__file__).resolve().parents[3]; N=Path(__file__).resolve().parent
B=R/'build/original-camera-director-owner-20260903'
(B/'ScenarioPublicationTests.cpp').write_text((N/'ScenarioPublicationTests.cpp').read_text())
for filename in ['parser-build.json','parser-fixture-build.json']:
    for row in json.loads((N/filename).read_text()):
        subprocess.run(row['command'],cwd=R/'pc-port',check=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
cmd=json.loads((N/'parser-link-command.json').read_text())
r=subprocess.run(cmd,cwd=R/'pc-port',text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
(N/'parser-link.log').write_text(r.stdout);r.check_returncode()
r=subprocess.run([str(B/'scenario-publication-tests')],cwd=R,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
(N/'parser-runtime.log').write_text(r.stdout);print(r.stdout,end='');r.check_returncode()
