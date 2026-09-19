#include "OriginalSphereQueryFixture.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/TriangleFilter.hpp"
#include "compat/CollisionDirectorOwnership.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageCollisionService.hpp"
#include <iostream>

namespace {
using Fixture = smgpc::test::OriginalSphereQueryFixture;
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
void near(float actual,float expected,const char* message) {
    require(std::isfinite(actual) && std::abs(actual-expected)<0.002f,message);
}
void feature_and_translation(Fixture& owner) {
    const TVec3f translation(30,40,50);
    Fixture::Part part(owner,Fixture::triangles(),Fixture::matrix(1,translation));
    struct Case { TVec3f center; u8 feature; } cases[] = {
        {TVec3f(2,.5f,2),1}, {TVec3f(-.25f,.25f,2),2}, {TVec3f(-.25f,.5f,-.25f),5}
    };
    for (const auto& item:cases) {
        require(Collision::checkStrikeBallToMap(item.center+translation,1,nullptr,nullptr)==1,
                "Original sphere query must retain face/edge/corner contacts");
        const auto* hit=Collision::getStrikeInfoMap(0);
        require(hit->mParentTriangle.mParts==&part.parts && hit->_88==item.feature,
                "Original part identity and exact contact feature must reach HitInfo");
        near(hit->mParentTriangle.getPos(0)->x,30,"Triangle vertex includes translation x");
        near(hit->mParentTriangle.getPos(0)->y,40,"Triangle vertex includes translation y");
        near(hit->mParentTriangle.getPos(0)->z,50,"Triangle vertex includes translation z");
        near(hit->mHitPos.y,40,"Projected contact uses world-space translation");
    }
}
void scale_and_thickness(Fixture& owner) {
    Fixture::Part part(owner,Fixture::triangles(2),Fixture::matrix(2));
    const TVec3f point(4,-2,4);
    require(Collision::checkStrikeBallToMap(point,1,nullptr,nullptr)==0,
            "Uniform model scale must not multiply the original world thickness limit");
    require(Collision::checkStrikeBallToMapWithThickness(point,1,4,nullptr,nullptr)==1,
            "Explicit thickness preserves its original world-space threshold");
    near(Collision::getStrikeInfoMap(0)->_60,3,"Scaled local depth converts back to world units");
}
void thickness_edge_rules(Fixture& owner) {
    Fixture::Part part(owner,Fixture::triangles());
    for (float height : {.1f, -.1f}) {
        const TVec3f point(-.5f,height,2);
        require(Collision::checkStrikeBallToMap(point,1,nullptr,nullptr)==0,
                "Ordinary sphere query rejects an edge farther sideways than the face distance");
        require(Collision::checkStrikeBallToMapWithThickness(point,1,2,nullptr,nullptr)==1,
                "Explicit thickness retains the retail edge overlap on either side of the face");
        const auto* hit=Collision::getStrikeInfoMap(0);
        require(hit->_88==2,"Thickness edge query preserves the original edge feature");
        near(hit->_60,std::sqrt(.75f)-height,"Thickness edge depth follows the circular cross-section");
    }
    require(Collision::checkStrikeBallToMapWithThickness(TVec3f(-.5f,-1,2),1,2,nullptr,nullptr)==0,
            "Thickness edge query rejects a sphere entirely behind the face cross-section");
}
void encounter_and_filter_capacity(Fixture& owner) {
    Fixture::Part part(owner,Fixture::triangles(2,{3,1,2}));
    const TVec3f point(2,.5f,2);
    require(Collision::checkStrikeBallToMap(point,1,nullptr,nullptr)==3,"All three original leaf prisms hit");
    require(Collision::getStrikeInfoMap(0)->mParentTriangle.mIdx==2 &&
            Collision::getStrikeInfoMap(1)->mParentTriangle.mIdx==0 &&
            Collision::getStrikeInfoMap(2)->mParentTriangle.mIdx==1,
            "Hit order must preserve original leaf encounter order, not prism identity sort");
    struct Filter final:TriangleFilterBase {
        bool isInvalidTriangle(const Triangle* triangle) const override { return triangle->mIdx!=1; }
    } filter;
    HitInfo hits[3];
    require(part.parts.checkStrikeBall(hits,2,point,1,false,&filter)==0,
            "Original KCL candidate capacity is consumed before triangle filtering");
    require(part.parts.checkStrikeBall(hits,3,point,1,false,&filter)==1 && hits[0].mParentTriangle.mIdx==1,
            "Larger original candidate capacity exposes the later accepted prism");
}
void moving_reaction(Fixture& owner) {
    Fixture::Part part(owner,Fixture::triangles());
    part.set_matrices(Fixture::matrix(1,TVec3f(0,4,0)),Fixture::matrix());
    part.parts._D4=0;
    const TVec3f point(2,.5f,2);
    require(Collision::checkStrikeBallToMap(point,1,nullptr,nullptr)==0,
            "Ordinary sphere query samples only current geometry");
    require(Collision::checkStrikeBallToMapWithMovingReaction(point,1,nullptr,nullptr)==1,
            "Moving query sweeps previous-to-current geometry and retains its earlier contact");
    const auto* hit=Collision::getStrikeInfoMap(0);
    near(hit->_7C.x,0,"Moving reaction is projected onto face normal x");
    near(hit->_7C.y,4,"Moving reaction preserves the original swept surface displacement");
    near(hit->_7C.z,0,"Moving reaction is projected onto face normal z");
    near(hit->_60,.5f,"Moving contact preserves depth at the first accepted sample");
    part.parts._D4=2;
    require(Collision::checkStrikeBallToMapWithMovingReaction(point,1,nullptr,nullptr)==0,
            "Original settled-part flag disables the sweep");
}
void point_boundaries_and_output(Fixture& owner) {
    Fixture::Part part(owner,Fixture::triangles(2,{3,1,2}));
    for (const TVec3f point : {TVec3f(0,-.5f,2),TVec3f(0,0,0),TVec3f(2,-2,2)}) {
        HitInfo hit;
        hit._88=42;
        hit._7C.set(3,4,5);
        owner.keeper->mHitInfoArray[0]._60=1234;
        require(Collision::checkStrikePointToMap(point,&hit)==1,
                "Original point query includes face, edge, vertex and back thickness boundaries");
        require(hit.mParentTriangle.mParts==&part.parts && hit.mParentTriangle.mIdx==2,
                "Point query returns first original leaf prism without identity sorting");
        near(hit._60,-point.y,"Point depth is signed face distance in world units");
        near(hit.mHitPos.y,0,"Point hit is projected onto the face");
        require(hit._88==42 && hit._7C.epsilonEquals(TVec3f(3,4,5),0.0001f),
                "Original point query leaves feature and unrelated moving reaction untouched");
        near(owner.keeper->mHitInfoArray[0]._60,1234,"Point output does not overwrite keeper sphere/line scratch");
    }
    HitInfo untouched;
    untouched._60=321;
    for (const TVec3f point : {TVec3f(-.01f,-.5f,2),TVec3f(2,.01f,2),TVec3f(2,-2.01f,2)}) {
        require(!MR::checkStrikePointToMap(point,&untouched),"Original point query rejects beyond each prism boundary");
        near(untouched._60,321,"Miss leaves caller's point output untouched");
        require(owner.keeper->_10==0,"Point miss resets original keeper hit count");
    }
    Fxyz huge;
    huge.x=1.0e30f;
    huge.y=0;
    huge.z=0;
    float distance=12;
    require(part.parts.mServer->checkPoint(&huge,1,&distance)==nullptr,
            "PPC saturated grid conversion rejects out-of-range point without a host float-to-int overflow");
    near(distance,12,"Out-of-range point leaves distance untouched");
}
void small_scale_point_rule(Fixture& owner) {
    Fixture::Part part(owner,Fixture::triangles(),Fixture::matrix(.5f));
    HitInfo hit;
    require(Collision::checkStrikePointToMap(TVec3f(1,.5f,1),&hit)==1,
            "Original sub-unit-scale part uses its thickness-sphere point branch");
    near(hit._60,-.5f,"Small-scale point branch retains signed depth even on the front side");
    near(hit.mHitPos.y,0,"Small-scale point hit still projects onto actual plane");
}
void fast_line_enclosed_surface(Fixture& owner) {
    Fixture::Part near_part(owner,Fixture::triangles());
    Fixture::Part reverse_side(owner,Fixture::triangles(),{1,0,0,0,0,-1,0,-2,0,0,-1,4});
    Fixture::Part far_part(owner,Fixture::triangles(),Fixture::matrix(1,TVec3f(0,-100,0)));
    TVec3f hit;
    Triangle triangle;
    const TVec3f start(2,10,2),offset(0,-200,0);
    require(MR::getFirstPolyOnLineToMap(&hit,&triangle,start,offset) && triangle.mParts==&near_part.parts,
            "Ordinary first line query selects the nearest surface");
    require(MR::getFirstPolyOnLineBFast(start,offset,&hit,&triangle) && triangle.mParts==&far_part.parts,
            "Original BFast normal probe rejects an enclosed nearer surface and continues to the next");
    near(hit.y,-100,"BFast returns the farther exposed plane");
}
void fast_line_segment_capacity(Fixture& owner) {
    std::vector<std::unique_ptr<Fixture::Part>> far_parts;
    for (int i=0;i<32;++i)
        far_parts.push_back(std::make_unique<Fixture::Part>(owner,Fixture::triangles(),Fixture::matrix(1,TVec3f(0,-6000,0))));
    Fixture::Part near_part(owner,Fixture::triangles());
    const TVec3f start(2,10,2),offset(0,-7000,0);
    Triangle triangle;
    require(MR::getFirstPolyOnLineToMap(nullptr,&triangle,start,offset) && triangle.mParts==&far_parts.front()->parts,
            "Control query demonstrates the original 32-hit full-line encounter limit");
    const auto* camera=MR::getCameraPolyFast(start,offset,nullptr);
    require(camera && camera->mParts==&near_part.parts,
            "Camera fast segmentation keeps distant parts from exhausting the first 5000-unit segment");
    require(MR::getFirstPolyOnLineBFast(start,offset,nullptr,&triangle) && triangle.mParts==&near_part.parts,
            "BFast uses the same original segment-local capacity");
}
void excluded_sensor_line(Fixture& owner) {
    require(MR::createSceneObj(SceneObj_SensorHitChecker)!=nullptr,"Sensor identity fixture needs the original checker");
    HitSensor near_sensor(0,0,1,nullptr),far_sensor(0,0,1,nullptr);
    Fixture::Part near_part(owner,Fixture::triangles(),Fixture::matrix(),&near_sensor);
    Fixture::Part far_part(owner,Fixture::triangles(),Fixture::matrix(1,TVec3f(0,-10,0)),&far_sensor);
    TVec3f normal,hit;
    const TVec3f start(2,10,2),offset(0,-30,0);
    require(MR::getFirstPolyNormalOnLineToMap(&normal,start,offset,&hit,&near_sensor),
            "Sensor-excluded normal query continues to an eligible farther part");
    near(hit.y,-10,"Excluded first surface does not hide the eligible farther surface");
    const auto* camera=MR::getCameraPolyFast(start,offset,&near_sensor);
    require(camera && camera->mParts==&far_part.parts,"Camera fast excludes the sensor before selecting nearest stored result");
}
void area_original_membership(Fixture& owner) {
    Fixture::Part first(owner,Fixture::triangles(2,{3,1,3,2}));
    Fixture::Part second(owner,Fixture::triangles());
    Fixture::Part third(owner,Fixture::triangles());
    std::array<Triangle,8> pairs,arrays;
    auto box=std::array{TVec3f(-1,-1,-1),TVec3f(11,1,11)};
    require(MR::createAreaPolygonList(pairs.data(),pairs.size(),box[0],box[1])==5 &&
            MR::createAreaPolygonListArray(arrays.data(),arrays.size(),box.data(),box.size())==5,
            "Both area APIs query actual keeper members and suppress repeated KCL prism pointers");
    for (int i=0;i<5;++i)
        require(pairs[i].mParts==arrays[i].mParts && pairs[i].mIdx==arrays[i].mIdx,
                "Area pair and array entry points preserve identical original part/prism order");
    require(pairs[0].mIdx==2 && pairs[1].mIdx==0 && pairs[2].mIdx==1,
            "Area query retains leaf encounter order and duplicate suppression");
    first.parts._CC=false;
    require(MR::createAreaPolygonList(pairs.data(),1,box[0],box[1])==1 && pairs[0].mParts==&second.parts,
            "Disabled original parts do not consume area output capacity");
    owner.keeper->removeFromZone(&second.parts,0);
    owner.keeper->addToZone(&second.parts,0);
    require(MR::createAreaPolygonList(pairs.data(),1,box[0],box[1])==1 && pairs[0].mParts==&third.parts,
            "Area query sees original erase-swap and append membership immediately");
    first.parts._CC=true;
    require(MR::createAreaPolygonList(pairs.data(),2,box[0],box[1])==2 && pairs[1].mIdx==0,
            "Area capacity truncates the original prism encounter sequence");
}
void area_transform_and_contract(Fixture& owner) {
    Fixture::Part part(owner,Fixture::triangles(),{0,-1,0,30,1,0,0,40,0,0,1,50});
    auto box=std::array{TVec3f(29,39,49),TVec3f(31,51,61)};
    Triangle pair,array;
    require(MR::createAreaPolygonList(&pair,1,box[0],box[1])==1 &&
            MR::createAreaPolygonListArray(&array,1,box.data(),box.size())==1,
            "Both area paths transform endpoints through actual inverse part matrix");
    require(pair.mParts==&part.parts && array.mParts==&part.parts && pair.mIdx==array.mIdx,
            "Transformed area results retain actual part identity");
    near(pair.mPos[0].x,30,"Transformed area triangle includes world translation x");
    near(pair.mPos[0].y,40,"Transformed area triangle includes world translation y");
    near(pair.mPos[0].z,50,"Transformed area triangle includes world translation z");
    require(MR::createAreaPolygonList(nullptr,0,box[0],box[1])==0,
            "Native zero output capacity does not enter the original fixed stack buffer query");
    const auto rejects=[](auto invoke) {
        try { invoke(); } catch (const std::invalid_argument&) { return; }
        throw std::runtime_error("Native area query accepted an invalid fixed-buffer contract");
    };
    rejects([&] { MR::createAreaPolygonList(&pair,513,box[0],box[1]); });
    rejects([&] { MR::createAreaPolygonListArray(&array,1,box.data(),33); });
    rejects([&] { MR::createAreaPolygonListArray(&array,1,nullptr,1); });
}
void publication_before_culling(Fixture& owner) {
    require(MR::createSceneObj(SceneObj_SensorHitChecker)!=nullptr,
            "Publication fixture requires the original sensor checker");
    auto* director=static_cast<CollisionDirector*>(MR::getSceneObjHolder()->getObj(SceneObj_CollisionDirector));
    auto* native_owner=smgpc::scene::current_collision_director_ownership();
    require(director && native_owner,"Publication fixture requires actual original and native scene owners");
    require(director->getCategoryKeeper(0)==owner.keeper,"Publication fixture must use the same original director");
    smgpc::scene::StageCollisionService map_service;
    map_service.activate();
    for (int category=0;category<4;++category) {
        auto* keeper=director->getCategoryKeeper(category);
        if (category!=0) {
            keeper->mZones[0]=new CollisionZone(0);
            keeper->mZoneNum=1;
            keeper->_A0=true;
        }
        auto& service=category==0 ? map_service : native_owner->category_service(category);
        struct Geometry {
            KCLFile file{};
            std::array<TVec3f,1> positions;
            std::array<TVec3f,4> normals;
            std::array<KC_PrismData,2> prisms;
            std::array<u16,4> octree{0x8000,2,1,0};
            explicit Geometry(const KCLFile& source) : file(source) {
                std::copy_n(source.mPos,positions.size(),positions.begin());
                std::copy_n(source.mNorms,normals.size(),normals.begin());
                std::copy_n(source.mPrisms,prisms.size(),prisms.begin());
                file.mPos=positions.data(); file.mNorms=normals.data();
                file.mPrisms=prisms.data(); file.mOctree=octree.data();
            }
        };
        smgpc::resource::KCollisionResource original(Fixture::triangles());
        auto geometry=std::make_shared<Geometry>(*original.native_file());
        auto resource=std::make_shared<smgpc::resource::GeneratedKCollisionResource>(
            geometry->file,geometry->positions,geometry->normals,geometry->prisms,geometry->octree,geometry);
        HitSensor sensor(0,0,1,nullptr);
        CollisionParts parts;
        std::unique_ptr<KCollisionServer> server(parts.mServer);
        std::unique_ptr<JMapInfo> attributes(parts.mServer->mapInfo);
        parts.mServer->init(resource->native_file(),nullptr);
        parts.mHitSensor=&sensor; parts._CC=true; parts._D4=2; parts._D8=10;
        parts.mKeeperIndex=category; parts.mZone=keeper->mZones[0];
        keeper->addToZone(&parts,0);
        struct Cleanup {
            CollisionCategorizedKeeper& keeper;
            CollisionParts& parts;
            smgpc::scene::StageCollisionService& service;
            ~Cleanup() { service.clear(); keeper.removeFromZone(&parts,0); }
        } cleanup{*keeper,parts,service};
        auto registration=std::make_shared<smgpc::scene::StageCollisionRegistrationState>(nullptr,&parts);
        require(service.register_generated_kcl(resource,*parts.mServer,Fixture::matrix(),
                    "publication boundary triangle",registration,&sensor,0).accepted,
                "Generated geometry must register against its actual original part and keeper");
        service.build();
        // These queries are outside the part sphere. Without the entry guard,
        // original culling never calls a CollisionParts narrow-phase guard.
        std::array box{TVec3f(10000),TVec3f(10001)};
        const TVec3f offset(1,0,0);
        HitInfo hit;
        Triangle triangle;
        const auto queries=[&](bool unavailable) {
            const auto check=[&](auto query) {
                bool rejected=false;
                unsigned result=1;
                try { result=query(); } catch (const std::logic_error&) { rejected=true; }
                require(rejected==unavailable && (rejected || result==0),
                        "Every off-bounds original query must honor category publication before culling");
            };
            check([&] { return keeper->checkStrikePoint(box[0],&hit); });
            check([&] { return keeper->checkStrikeBall(box[0],1,false,nullptr,nullptr); });
            check([&] { return keeper->checkStrikeBallWithThickness(box[0],1,2,nullptr,nullptr); });
            check([&] { return keeper->checkStrikeLine(box[0],offset,1,nullptr,nullptr); });
            check([&] { return keeper->createAreaPolygonList(&triangle,1,box[0],box[1]); });
            check([&] { return keeper->createAreaPolygonListArray(&triangle,1,box.data(),box.size()); });
        };
        queries(false);
        const auto invalidate=[&] {
            geometry->prisms[1].mNormalIndex=99;
            bool rejected=false;
            try { service.update_registered_geometry(*registration); }
            catch (const std::invalid_argument&) { rejected=true; }
            require(rejected,"Invalid generated index must fail publication");
        };
        invalidate();
        queries(true);
        for (int other=0;other<4;++other)
            if (other!=category)
                require(director->getCategoryKeeper(other)->checkStrikeBall(box[0],1,false,nullptr,nullptr)==0,
                        "Failed publication must not quarantine an unrelated collision category");
        registration->set_enabled(false); parts._CC=false;
        queries(false);
        registration->set_enabled(true); parts._CC=true;
        queries(true);
        geometry->prisms[1].mNormalIndex=0;
        service.update_registered_geometry(*registration);
        queries(false);
        invalidate();
        registration->release_owner(); parts._CC=false;
        queries(false);
        std::cout<<"PASS off-bounds publication boundary category "<<category<<'\n';
    }
    map_service.deactivate();
}
}
int main() {
    try {
        Fixture fixture;
        feature_and_translation(fixture);
        scale_and_thickness(fixture);
        thickness_edge_rules(fixture);
        encounter_and_filter_capacity(fixture);
        moving_reaction(fixture);
        point_boundaries_and_output(fixture);
        small_scale_point_rule(fixture);
        fast_line_enclosed_surface(fixture);
        fast_line_segment_capacity(fixture);
        excluded_sensor_line(fixture);
        area_original_membership(fixture);
        area_transform_and_contract(fixture);
        publication_before_culling(fixture);
        require(fixture.keeper->mZoneCount==0,"All synthetic parts retire from original keeper");
        std::cout<<"PASS original map queries: sphere features/translation/thickness/order/filtering/motion, point boundaries/scale/output, segmented fast lines/exclusion/enclosure and retirement\n";
    } catch (const std::exception& error) {
        std::cerr<<"FAIL original sphere queries: "<<error.what()<<'\n';
        return 1;
    }
}
