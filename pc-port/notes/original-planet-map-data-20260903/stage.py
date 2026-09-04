from pathlib import Path
R=Path(__file__).resolve().parents[3];B=R/'build/original-planet-map-data-20260903';S=B/'staged';S.mkdir(exist_ok=True)
def extract(text,sig):
 start=text.index(sig);brace=text.index('{',start);i=brace+1;depth=1
 while depth:
  depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[start:i]
def put(rel,s):
 p=S/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s)
root=(R/'src/Game/Map/PlanetMapCreator.cpp').read_text();table=root[root.index('    static const UniqueEntry'):root.index('    void makeSubModelName')]
helpers=[extract(root,sig) for sig in ['    void makeSubModelName(', '    static bool isDataForceLow(']]
sigs=['PlanetMapCreator::PlanetMapCreator(','void PlanetMapCreator::makeArchiveListPlanet(','void PlanetMapCreator::createPlanetMapDataTable(','void PlanetMapCreator::addTableData(','PlanetMapData* PlanetMapCreator::getTableData(','bool PlanetMapCreator::isScenarioForceLow(','void PlanetMapCreatorFunction::makeArchiveList(','bool PlanetMapCreatorFunction::isLoadArchiveAfterScenarioSelected(','bool PlanetMapCreatorFunction::isRegisteredObj(']
put('compat/OriginalPlanetMapData.cpp', '#include "Game/Map/PlanetMapCreator.hpp"\n#include "Game/NameObj/NameObjArchiveListCollector.hpp"\n#include "Game/Scene/SceneObjHolder.hpp"\n#include "Game/Util/ObjUtil.hpp"\n#include "Game/Util/ModelUtil.hpp"\n#include "Game/Util/SceneUtil.hpp"\n#include "Game/Util/StringUtil.hpp"\n#include <cstdio>\n#include <cstring>\n\nnamespace {\n'+table+'\n\n'.join(helpers)+'\n}\n\n'+'\n\n'.join(extract(root,sig) for sig in sigs)+'\n')
put('Game/Map/PlanetMapCreator.hpp',(R/'include/Game/Map/PlanetMapCreator.hpp').read_text())
collector=(R/'src/Game/NameObj/NameObjArchiveListCollector.cpp').read_text().replace('#include "Game/Util.hpp"','#include "Game/Util/StringUtil.hpp"');put('Game/NameObj/NameObjArchiveListCollector.cpp',collector)
s=(R/'pc-port/src/Game/Util/ObjUtil.hpp').read_text().replace('    JMapInfo* createCsvParser(const ResourceHolder*, const char*, ...);','    JMapInfo* createCsvParser(const ResourceHolder*, const char*, ...);\n    JMapInfo* createCsvParser(const char*, const char*, ...);');put('Game/Util/ObjUtil.hpp',s)
put('compat/OriginalArchiveCsvReader.cpp','''#include "Game/Util/ObjUtil.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include <stdexcept>

namespace MR {
    JMapInfo* createCsvParser(const char* pArchive, const char* pFormat, ...) {
        auto* service = smgpc::compat::ResourceHolderService::active();
        if (!service) throw std::logic_error("Archive CSV reader requires the actual resource-holder owner");
        ResourceHolder* pResourceHolder = service->create_and_add(pArchive);

        return MR::createCsvParser(pResourceHolder, pFormat);
    }
}
''')
model=(R/'src/Game/Util/ModelUtil.cpp').read_text();put('compat/OriginalModelExistence.cpp','#include "Game/Util/ModelUtil.hpp"\n#include "Game/Util/FileUtil.hpp"\n#include <cstdio>\n\nnamespace MR {\n'+extract(model,'    bool isExistModel(')+'\n}\n')
