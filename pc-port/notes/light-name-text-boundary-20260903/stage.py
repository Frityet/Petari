#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[3];B=R/'build/light-name-text-boundary-20260903';S=B/'staged'
def put(rel,text):
 p=S/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
s=(R/'pc-port/src/render/light/LightData.cpp').read_text().replace('#include "resource/TextEncoding.hpp"\n','').replace('''            _area_light_names.push_back(smgpc::resource::decode_cp932(
                light_table.get_string(row, "AreaLightName").value_or(std::string {})));''','''            _area_light_names.push_back(
                light_table.get_string(row, "AreaLightName").value_or(std::string {}));''').replace('''                    .area_light_name = smgpc::resource::decode_cp932(
                        zone_table.get_string(row, "AreaLightName").value_or(std::string {})),''','''                    .area_light_name = zone_table.get_string(row, "AreaLightName").value_or(std::string {}),''');assert 'decode_cp932' not in s;put('src/render/light/LightData.cpp',s)
s=(R/'pc-port/src/render/light/LightData.hpp').read_text().replace('        [[nodiscard]] AreaLightInfo *area_light_info', '        // Game-facing names retain authored CP932 bytes. Decode only for host presentation.\n        [[nodiscard]] AreaLightInfo *area_light_info');put('src/render/light/LightData.hpp',s)
s=(R/'pc-port/tests/AreaObjRealOrAbsentTests.cpp').read_text().replace('#include "render/light/LightData.hpp"','#include "render/light/LightData.hpp"\n#include "resource/TextEncoding.hpp"')
for expr in ['root->mAreaLightName','light_data.default_area_light_name()','child->mAreaLightName','observatory->mAreaLightName','next_scene_light->mAreaLightName']:s=s.replace('std::string_view('+expr+')','smgpc::resource::decode_cp932('+expr+')')
put('tests/AreaObjRealOrAbsentTests.cpp',s)

put('tests/LightNameTextBoundaryTests.cpp',(Path(__file__).resolve().parent/'LightNameTextBoundaryTests.cpp').read_text())
