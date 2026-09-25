from pathlib import Path
import collections
import difflib
import hashlib
import json
import re

root = Path(__file__).resolve().parents[3]
notes = Path(__file__).resolve().parent
owners = [
    "src/JSystem/J3DGraphBase/J3DMaterial.cpp",
    "src/JSystem/J3DGraphBase/J3DMatBlock.cpp",
    "src/JSystem/J3DGraphBase/J3DTevs.cpp",
    "src/JSystem/J3DGraphAnimator/J3DMaterialAnm.cpp",
    "src/JSystem/J3DGraphAnimator/J3DMaterialAttach.cpp",
    "src/JSystem/J3DGraphAnimator/J3DShapeTable.cpp",
]
providers = [
    "J3DMaterialFactoryCompat", "J3DMaterialHelpersCompat", "J3DMaterialVariantsCompat",
    "J3DMatBlockCompat", "J3DTevsCompat", "J3DTexMtxCompat",
    "J3DMaterialAnmCompat", "J3DMaterialAttachCompat", "J3DShapeTableCompat",
]
allowed_adaptations = {
    "loadMatColors", "loadAmbColors", "getTexNoReg",
    "J3DTexMtx::loadTexMtx", "J3DGDLoadTexMtxImm",
}
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

checks = []
def check(name, passed, evidence=None):
    checks.append({"name": name, "passed": bool(passed), "evidence": evidence})

expected_symbols = set()
comparisons = []
for path in owners:
    native_text = (root / path).read_text()
    donor_text = (root / "decomp" / path).read_text()
    native = {f["symbol"]: f for f in functions(native_text)}
    donor = {f["symbol"]: f for f in functions(donor_text)}
    different = [name for name in native if name not in donor or native[name]["normalized"] != donor[name]["normalized"]]
    check(path + ": complete donor function set", native.keys() == donor.keys())
    check(path + ": only documented native body adaptations", set(different) <= allowed_adaptations, different)
    expected_symbols.update(native)
    comparisons.append({"path": path, "functions": len(native), "nonexact_body_symbols": different})
check("nine old providers deleted", all(not (root / "src/compat" / (name + ".cpp")).exists() for name in providers))
definitions = collections.defaultdict(list)
for directory in [root / "src/compat", root / "src/JSystem"]:
    for path in directory.rglob("*.cpp"):
        for f in functions(path.read_text()):
            if f["symbol"] in expected_symbols:
                definitions[f["symbol"]].append(str(path.relative_to(root)))
collisions = {name: definitions[name] for name in expected_symbols if len(definitions[name]) != 1}
check("one CPP provider for each restored symbol", not collisions, collisions)
block = (root / owners[1]).read_text()
tevs = (root / owners[2]).read_text()
header = (root / "src/JSystem/J3DGraphBase/J3DMatBlock.hpp").read_text()
check("both original diffLight overrides declared", header.count("virtual void diffLight() override;") == 2 and "diffColorChan" not in header)
check("light-count bits reach original light upload", "if ((diffFlags & J3DDiffFlag_ColorChan) || ((diffFlags >> 4) & 0xF))\n        diffLight();" in block and "mLight[i]->load(i);" in block)
check("packed RGBA uses endian reads", block.count("aurora::endian::read_u32(color") == 4)
check("unaligned texture register uses endian read", "aurora::endian::read_u32((u8*)pDL + 1)" in tevs)
check("original signed LOD casts retained", "static_cast< s8 >(resTIMG->mMinLod)" in tevs and "static_cast< s8 >(resTIMG->mMaxLod)" in tevs)
check("PPC texture matrix FP contract boundary retained", "#pragma clang fp contract(off)" in tevs)
def light_default(text):
    return normalize(text[text.index("const J3DLightInfo j3dDefaultLightInfo"):].split("};", 1)[0])
check("default light data preserved from prior provider", light_default(tevs) == light_default((notes / "baseline/src/compat/J3DTevsCompat.cpp").read_text()))
material = (root / owners[0]).read_text()
material_donor = (root / "decomp" / owners[0]).read_text()
check("native allocation sizing retained", material.count("sizeof(") > 0 and material.count("sizeof(") == material_donor.count("sizeof("))
cpp_text = "\n".join(path.read_text() for path in (root / "src").rglob("*.cpp"))
check("single original TEV-order assignment provider", cpp_text.count("J3DTevOrder& J3DTevOrder::operator=") == 1)
check("single GD command header provider", len(re.findall(r"(?:inline )?void J3DGDWriteXFCmdHdr\([^;]+?\)\s*\{", cpp_text)) == 1)
check("display-list device scope untouched", not any("J3DPacket" in row["path"] for row in json.loads((notes / "before.json").read_text())["files"]))
baseline = json.loads((notes / "before.json").read_text())
check("all touched existing files were clean before changes", all(not row["before_status"] for row in baseline["files"]))
patch = []
for row in baseline["files"]:
    path = row["path"]
    before = (notes / "baseline" / path).read_text()
    after = (root / path).read_text() if (root / path).exists() else ""
    patch.extend(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True), fromfile="a/"+path, tofile="b/"+path if after else "/dev/null"))
for path in owners:
    after = (root / path).read_text()
    patch.extend(difflib.unified_diff([], after.splitlines(keepends=True), fromfile="/dev/null", tofile="b/"+path))
(notes / "source-changes.patch").write_text("".join(patch))
check("no added trailing whitespace", all(line[1:].rstrip() == line[1:] for line in "".join(patch).splitlines() if line.startswith("+") and not line.startswith("+++")))
result = {"validation": "source only; no build or runtime execution", "comparisons": comparisons, "checks": checks}
(notes / "source-validation.json").write_text(json.dumps(result, indent=2)+"\n")
failed = [entry["name"] for entry in checks if not entry["passed"]]
print(json.dumps({"checks": len(checks), "functions": len(expected_symbols), "failed": failed}, indent=2))
raise SystemExit(bool(failed))
