#!/usr/bin/env python3
from pathlib import Path
import json,hashlib,re
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-data-20260903'
def extract(text,signature):
 start=text.index(signature);i=text.index('{',start)+1;depth=1
 while depth:
  depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[start:i]
def squash(s):return re.sub(r'\s+','',s)
def main():
 root=R/'src/Game/Map/PlanetMapCreator.cpp';native=B/'staged/compat/OriginalPlanetMapData.cpp';records=[]
 signatures=['void makeSubModelName(','static bool isDataForceLow(','PlanetMapCreator::PlanetMapCreator(','void PlanetMapCreator::makeArchiveListPlanet(','void PlanetMapCreator::createPlanetMapDataTable(','void PlanetMapCreator::addTableData(','PlanetMapData* PlanetMapCreator::getTableData(','bool PlanetMapCreator::isScenarioForceLow(','void PlanetMapCreatorFunction::makeArchiveList(','bool PlanetMapCreatorFunction::isLoadArchiveAfterScenarioSelected(','bool PlanetMapCreatorFunction::isRegisteredObj(']
 for sig in signatures:
  assert squash(extract(root.read_text(),sig))==squash(extract(native.read_text(),sig));records.append(dict(signature=sig,complete_body_identical=True))
 a=root.read_text();b=native.read_text();a=a[a.index('static const UniqueEntry'):a.index('    void makeSubModelName')];b=b[b.index('static const UniqueEntry'):b.index('    void makeSubModelName')];assert squash(a)==squash(b)
 for file,other,sigs in [('src/Game/NameObj/NameObjArchiveListCollector.cpp','Game/NameObj/NameObjArchiveListCollector.cpp',['NameObjArchiveListCollector::NameObjArchiveListCollector(','void NameObjArchiveListCollector::addArchive(','const char* NameObjArchiveListCollector::getArchive(']),('src/Game/Util/ModelUtil.cpp','compat/OriginalModelExistence.cpp',['bool isExistModel('])]:
  for sig in sigs:assert squash(extract((R/file).read_text(),sig))==squash(extract((B/'staged'/other).read_text(),sig));records.append(dict(signature=sig,complete_body_identical=True))
 assert (R/'include/Game/Map/PlanetMapCreator.hpp').read_bytes()==(B/'staged/Game/Map/PlanetMapCreator.hpp').read_bytes()
 (N/'source-evidence.json').write_text(json.dumps(dict(root_sha256=hashlib.sha256(root.read_bytes()).hexdigest(),native_sha256=hashlib.sha256(native.read_bytes()).hexdigest(),bodies=records,archive_table_and_static_names_identical=True,planet_header_identical=True,archive_csv_boundary='Only ResourceHolderManager singleton lookup is replaced by the existing ResourceHolderService actual owner; original resource-holder overload call preserved.'),indent=2)+'\n')
if __name__=='__main__':main()
