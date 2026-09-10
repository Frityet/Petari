from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=root/'src/compat/DemoSceneRuntime.cpp';old=p.read_text();(Path(__file__).parent/'DemoSceneRuntime-before.cpp').write_text(old)
# Retain only read-only authored-row discovery; the original objects own all behavior.
seeds=old[old.index('        constexpr auto cDemoSheetArchivePath'):old.index('        [[nodiscard]] std::string actor_name')]
tail=old[old.index('    DemoSceneRuntime *active_demo_scene_runtime()'):]
head='''#include "compat/DemoSceneRuntime.hpp"
#include "compat/DemoDirectorOwnership.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/DemoExecutor.hpp"
#include "Game/Demo/DemoExecutorFunction.hpp"
#include "Game/Demo/DemoCastGroupHolder.hpp"
#include "Game/Demo/DemoCastSubGroup.hpp"
#include "Game/Demo/DemoActionKeeper.hpp"
#include "Game/Demo/DemoPlayerKeeper.hpp"
#include "Game/Demo/DemoTimeKeeper.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Demo/DemoStartRequestHolder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "runtime/RuntimeServices.hpp"
#include "resource/RarcArchive.hpp"
#include <aurora/exception.hpp>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>

namespace smgpc::compat {
namespace {
'''
body='''
    auto& installed_runtimes() { static std::vector<DemoSceneRuntime*> runtimes; return runtimes; }
    bool same(const char* value, std::string_view expected) { return value && expected == value; }
    std::optional<std::string_view> view(const char* value) { return value ? std::optional<std::string_view>(value) : std::nullopt; }
}
struct DemoSceneRuntime::Impl {
    DemoDirector* director = nullptr;
    std::vector<DemoSceneDefinition> definitions;
    std::vector<DemoSceneSubGroupDefinition> subgroups;
    std::vector<DemoExecutor*> executors;
    // Cast identities are observations for diagnostics only. Original Game
    // arrays own membership, callback registration and execution.
    struct CastIdentity { const LiveActor* actor; std::size_t definition; s32 id; };
    std::vector<CastIdentity> identities;

    Impl(const resource::RarcArchive& archive, std::span<const scene::StagePlacementObject> placements) {
        const JkrHostAllocationScope host;
        director = dynamic_cast<DemoDirector*>(MR::createSceneObj(SceneObj_DemoDirector));
        if (!director) aurora::throw_host_exception<std::logic_error>("Demo resources require the actual scene DemoDirector");
        auto* ownership = scene::current_demo_director_ownership();
        if (!ownership) aurora::throw_host_exception<std::logic_error>("Demo resources require native lifetime ownership");
        const auto seeds = collect_definition_seeds(placements);
        for (const auto& seed : seeds.definitions) {
            definitions.push_back({seed.zone_id, seed.group_link_id, seed.demo_name, seed.time_sheet_name,
                                   seed.switches, seed.source_table_path, seed.source_row,
                                   DemoSheetRuntime::load(archive, seed.time_sheet_name)});
        }
        for (const auto& seed : seeds.subgroups)
            subgroups.push_back({seed.zone_id, seed.group_link_id, seed.demo_name, seed.source_table_path, seed.source_row});
        for (const auto& placement : placements) {
            if (!is_demo_obj_info_path(placement.table_path) ||
                (placement.object_name != cPrimaryDemoObjectName && placement.object_name != cSubDemoObjectName)) continue;
            const auto marker = mark_name_obj_runtime_registrations();
            const JkrAllocationScope heap(scene::current_scene_allocation_domain());
            DemoCastGroup* group = placement.object_name == cPrimaryDemoObjectName
                ? static_cast<DemoCastGroup*>(new DemoExecutor("DemoGroup"))
                : static_cast<DemoCastGroup*>(new DemoCastSubGroup("DemoSubGroup"));
            // Adopt the actual root before initialization so a failed init is
            // still retired by the scene. The same boundary owns registered children.
            scene::adopt_current_scene_obj_holder_descendant(group);
            ownership->capture_cast_group(*group);
            if (auto* executor = dynamic_cast<DemoExecutor*>(group)) ownership->capture_executor(*executor);
            try { group->init(JMapInfoIter(&placement.jmap_info, placement.jmap_entry_index)); }
            catch (...) {
                for (auto* child : snapshot_name_obj_runtime_objects_since(marker))
                    if (!name_obj_runtime_ownership_is_claimed(child)) scene::adopt_current_scene_obj_holder_descendant(child);
                throw;
            }
            for (auto* child : snapshot_name_obj_runtime_objects_since(marker))
                if (!name_obj_runtime_ownership_is_claimed(child)) scene::adopt_current_scene_obj_holder_descendant(child);
            if (auto* executor = dynamic_cast<DemoExecutor*>(group)) executors.push_back(executor);
        }
    }
};
DemoSceneRuntime::DemoSceneRuntime(runtime::DvdFileSystemService& dvd,
    std::span<const scene::StagePlacementObject> placements, std::span<const scene::StageGeneralPos>, runtime::WipeService*)
    : _impl(std::make_unique<Impl>(dvd.archive(cDemoSheetArchivePath), placements)) {
    installed_runtimes().push_back(this);
    adopt_deferred_scene_simple_casts(*this);
}
DemoSceneRuntime::DemoSceneRuntime(const resource::RarcArchive& archive,
    std::span<const scene::StagePlacementObject> placements, std::span<const scene::StageGeneralPos>, runtime::WipeService*)
    : _impl(std::make_unique<Impl>(archive, placements)) {
    installed_runtimes().push_back(this);
    adopt_deferred_scene_simple_casts(*this);
}
DemoSceneRuntime::~DemoSceneRuntime() { std::erase(installed_runtimes(), this); }
void DemoSceneRuntime::movement() { _impl->director->movement(); }
std::span<const DemoSceneDefinition> DemoSceneRuntime::definitions() const { return _impl->definitions; }
std::span<const DemoSceneSubGroupDefinition> DemoSceneRuntime::subgroups() const { return _impl->subgroups; }
const DemoSceneDefinition* DemoSceneRuntime::definition(std::size_t i) const { return i < _impl->definitions.size() ? &_impl->definitions[i] : nullptr; }
std::optional<std::size_t> DemoSceneRuntime::find_definition(s32 zone, s32 link) const {
    for (std::size_t i=0;i<_impl->definitions.size();++i) if (_impl->definitions[i].zone_id==zone && _impl->definitions[i].group_link_id==link) return i;
    return std::nullopt;
}
std::optional<std::size_t> DemoSceneRuntime::find_definition(std::string_view name) const {
    for (std::size_t i=0;i<_impl->definitions.size();++i) if (_impl->definitions[i].demo_name==name) return i;
    return std::nullopt;
}
std::optional<std::size_t> DemoSceneRuntime::find_subgroup(s32 zone, s32 link) const {
    for (std::size_t i=0;i<_impl->subgroups.size();++i) if (_impl->subgroups[i].zone_id==zone && _impl->subgroups[i].group_link_id==link) return i;
    return std::nullopt;
}
std::optional<std::size_t> DemoSceneRuntime::find_subgroup(std::string_view name) const {
    for (std::size_t i=0;i<_impl->subgroups.size();++i) if (_impl->subgroups[i].demo_name==name) return i;
    return std::nullopt;
}
bool DemoSceneRuntime::try_register_cast(LiveActor* actor,const JMapInfoIter& iter) {
    const auto result = _impl->director->registerDemoCast(actor,iter);
    if (result) for(std::size_t i=0;i<_impl->executors.size();++i)
        if(DemoExecutorFunction::isRegisteredDemoCast(_impl->executors[i],actor)) _impl->identities.push_back({actor,i,MR::getDemoCastID(iter)});
    return result;
}
bool DemoSceneRuntime::try_register_cast(LiveActor* actor,std::string_view name,const JMapInfoIter& iter) {
    const std::string owned(name); const auto result = _impl->director->registerDemoCast(actor,owned.c_str(),iter);
    if (result) for(std::size_t i=0;i<_impl->executors.size();++i)
        if(DemoExecutorFunction::isRegisteredDemoCast(_impl->executors[i],actor)) _impl->identities.push_back({actor,i,MR::getDemoCastID(iter)});
    return result;
}
void DemoSceneRuntime::register_simple_cast(LiveActor* actor) { _impl->director->registerDemoSimpleCast(actor); }
void DemoSceneRuntime::register_simple_cast(LayoutActor* actor) { _impl->director->registerDemoSimpleCast(actor); }
void DemoSceneRuntime::register_simple_cast(NameObj* object) { _impl->director->registerDemoSimpleCast(object); }
void DemoSceneRuntime::release_actor(const LiveActor* actor) {
    std::erase_if(_impl->identities,[actor](const auto& value){return value.actor==actor;});
}
std::optional<DemoSheetStartResult> DemoSceneRuntime::start_demo(NameObj* starter,std::string_view name,
    std::optional<std::string_view> part,DemoPlayerMode mode) {
    const auto index=find_definition(name); if(!index) return std::nullopt;
    auto* executor=_impl->executors[*index];
    if(!executor->mTimeKeeper->mNumPartInfos) return DemoSheetStartResult::EmptyTimeTable;
    const char* part_name=nullptr;
    if(part) {
        for(s32 i=0;i<executor->mTimeKeeper->mNumPartInfos;++i)
            if(same(executor->mTimeKeeper->mMainPartInfos[i].mPartName,*part)) part_name=executor->mTimeKeeper->mMainPartInfos[i].mPartName;
        if(!part_name) return DemoSheetStartResult::PartNotFound;
    }
    if(mode==DemoPlayerMode::MarioPuppetable) MR::startTimeKeepDemoMarioPuppetable(starter,executor->mName,part_name);
    else MR::startTimeKeepDemo(starter,executor->mName,part_name);
    return DemoSheetStartResult::Started;
}
std::optional<DemoSheetStartResult> DemoSceneRuntime::start_demo_registered(LiveActor* starter,std::optional<std::string_view> part,DemoPlayerMode mode) {
    auto* executor=DemoFunction::findDemoExecutor(starter); if(!executor) return std::nullopt;
    return start_demo(starter,executor->mName,part,mode);
}
bool DemoSceneRuntime::stop_active_demo(const NameObj* starter,std::optional<std::string_view> name) {
    if(!is_active() || (name&&!is_active(*name))) return false;
    _impl->director->endDemo(const_cast<NameObj*>(starter),_impl->director->getCurrentDemoName(),false); return true;
}
void DemoSceneRuntime::release_puppetable_control(bool) { MarioAccess::endRemoteDemo(nullptr); }
void DemoSceneRuntime::pause_time_keep(const LiveActor* actor) { if(auto* e=DemoFunction::findDemoExecutorActive(actor)) e->pause(); }
void DemoSceneRuntime::resume_time_keep(const LiveActor* actor) { if(auto* e=DemoFunction::findDemoExecutorActive(actor)) e->resume(); }
bool DemoSceneRuntime::try_register_action_functor(const LiveActor* actor,const MR::FunctorBase& functor,std::optional<std::string_view> name) {
    const auto value=name?std::string(*name):std::string(); return MR::tryRegisterDemoActionFunctor(actor,functor,name?value.c_str():nullptr);
}
bool DemoSceneRuntime::try_register_action_functor(const LiveActor* actor,std::string_view demo,const MR::FunctorBase& functor,std::optional<std::string_view> name) {
    const auto value=name?std::string(*name):std::string();const std::string demo_name(demo);
    return MR::tryRegisterDemoActionFunctorDirect(actor,functor,demo_name.c_str(),name?value.c_str():nullptr);
}
bool DemoSceneRuntime::try_register_action_nerve(const LiveActor* actor,const Nerve* nerve,std::optional<std::string_view> name) {
    const auto value=name?std::string(*name):std::string();return MR::tryRegisterDemoActionNerve(actor,nerve,name?value.c_str():nullptr);
}
bool DemoSceneRuntime::has_action_capability(const LiveActor* actor,s32 type) const {
    const auto* executor=DemoFunction::findDemoExecutor(actor); return executor&&executor->mActionKeeper->isRegisteredDemoAction(actor,type);
}
bool DemoSceneRuntime::has_cast(const LiveActor* actor) const { return DemoFunction::findDemoExecutor(actor)!=nullptr; }
bool DemoSceneRuntime::has_cast(const LiveActor* actor,std::string_view name) const {const std::string value(name);return DemoFunction::isRegisteredDemoCast(actor,value.c_str());}
bool DemoSceneRuntime::is_active() const { return _impl->director->mIsActive; }
bool DemoSceneRuntime::is_active(std::string_view name) const {return (_impl->director->mExecutor&&same(_impl->director->mExecutor->mName,name))||same(_impl->director->getCurrentDemoName(),name);}
bool DemoSceneRuntime::is_time_keep_active() const {return is_active()&&_impl->director->mExecutor;}
bool DemoSceneRuntime::is_active_registered(const LiveActor* actor) const {const auto*e=DemoFunction::findDemoExecutor(actor);return e&&e==_impl->director->mExecutor;}
bool DemoSceneRuntime::registered_demo_has_player_rows(const LiveActor* actor) const {const auto*e=DemoFunction::findDemoExecutor(actor);return e&&e->mPlayerKeeper->mNumPlayerInfos>0;}
bool DemoSceneRuntime::part_exists(const LiveActor* actor,std::string_view part) const {const auto*e=DemoFunction::findDemoExecutor(actor);const std::string name(part);return e&&DemoExecutorFunction::isExistDemoPart(e,name.c_str());}
bool DemoSceneRuntime::is_part_active(std::string_view part) const {const std::string name(part);return DemoFunction::isDemoPartActiveFunction(name.c_str());}
bool DemoSceneRuntime::is_demo_last_step() const {return DemoFunction::isDemoLastPartLastStep();}
std::optional<s32> DemoSceneRuntime::part_step(std::string_view part) const {if(!is_part_active(part))return std::nullopt;const std::string name(part);return DemoFunction::getDemoPartStepFunction(name.c_str());}
std::optional<s32> DemoSceneRuntime::part_total_step(std::string_view part) const {if(!is_part_active(part))return std::nullopt;const std::string name(part);return DemoFunction::getDemoPartTotalStepFunction(name.c_str());}
std::optional<std::string_view> DemoSceneRuntime::current_main_part_name(std::string_view demo) const {const std::string name(demo);auto*e=DemoFunction::findDemoExecutor(name.c_str());return e&&e->mTimeKeeper->mSubPartInfos?view(e->mTimeKeeper->mSubPartInfos->mPartName):std::nullopt;}
std::string_view DemoSceneRuntime::active_demo_name() const {return view(_impl->director->getCurrentDemoName()).value_or(std::string_view{});}
std::size_t DemoSceneRuntime::membership_count(const LiveActor* actor) const {std::size_t count=0;for(auto*e:_impl->executors)count+=DemoExecutorFunction::isRegisteredDemoCast(e,actor);return count;}
std::size_t DemoSceneRuntime::subgroup_membership_count(const LiveActor* actor) const {std::size_t count=0;auto*g=_impl->director->mCastSubGroupHolder;for(s32 i=0;i<g->getObjNum();++i){auto* group=g->getCastGroup(i)->mGroup;for(s32 j=0;j<group->getObjNum();++j)if(group->getActor(j)==actor){++count;break;}}return count;}
std::size_t DemoSceneRuntime::simple_cast_registration_count(const NameObj* object) const {return scene::current_demo_director_ownership()->simple_cast_registration_count(object);}
namespace {
std::size_t action_count_for(const DemoExecutor* e,const LiveActor* actor,int kind) {
    if(!e)return 0;std::size_t count=0;for(s32 i=0;i<e->mActionKeeper->mNumInfos;++i){const auto* row=e->mActionKeeper->mInfoArray[i];for(s32 j=0;j<row->mCastCount;++j)if(row->mCastList[j]==actor&&(kind==0||(kind==1&&row->mFunctors[j])||(kind==2&&row->mNerves[j])))++count;}return count;
}
}
std::size_t DemoSceneRuntime::action_count(const LiveActor* actor) const {std::size_t count=0;for(auto*e:_impl->executors)count+=action_count_for(e,actor,0);return count;}
std::size_t DemoSceneRuntime::action_count(const LiveActor* actor,std::string_view demo) const {const std::string name(demo);return action_count_for(DemoFunction::findDemoExecutor(name.c_str()),actor,0);}
std::size_t DemoSceneRuntime::functor_count(const LiveActor* actor,std::string_view demo) const {const std::string name(demo);return action_count_for(DemoFunction::findDemoExecutor(name.c_str()),actor,1);}
std::size_t DemoSceneRuntime::nerve_count(const LiveActor* actor,std::string_view demo) const {const std::string name(demo);return action_count_for(DemoFunction::findDemoExecutor(name.c_str()),actor,2);}
std::optional<s32> DemoSceneRuntime::cast_id(const LiveActor* actor,std::size_t i) const {for(const auto& value:_impl->identities)if(value.actor==actor&&value.definition==i)return value.id;return std::nullopt;}
std::string_view DemoSceneRuntime::cast_name(const LiveActor* actor,std::size_t i) const {return i<_impl->executors.size()&&DemoExecutorFunction::isRegisteredDemoCast(_impl->executors[i],actor)?actor->getName():std::string_view{};}
void dispatch_demo_wipe_row(const DemoWipeRow& row,runtime::WipeService& wipe) {
    const auto name=resource::decode_cp932(row.wipe_name);
    switch(row.wipe_type){case 0:wipe.open(name,row.wipe_frame);break;case 1:wipe.close(name,row.wipe_frame);break;case 2:wipe.force_open(name);break;case 3:wipe.force_close(name);break;default:break;}
}
'''
p.write_text(head+seeds+body+tail)
p=root/'src/compat/DemoSceneRuntime.hpp';s=p.read_text();a=s.index('    // Name arguments,');b=s.index('    public:',a);s=s[:a]+'''    // Retains authored placement diagnostics and creates complete original
    // Game owners. DemoDirector/Executor own scheduling, membership and clocks.
    class DemoSceneRuntime final {
'''+s[b:];s=s.replace('~DemoSceneRuntime() override;','~DemoSceneRuntime();').replace('void movement() override;','void movement();');p.write_text(s)
