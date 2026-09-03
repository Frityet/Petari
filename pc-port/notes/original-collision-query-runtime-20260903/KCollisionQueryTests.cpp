#define main original_resource_fixture_main
#include "../../pc-port/tests/OriginalKCollisionResourceTests.cpp"
#undef main
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

namespace {
Bytes query_resource() {
    Bytes tree(52);
    put32(tree, 0, 4);
    for (unsigned child = 0; child < 8; ++child)
        put32(tree, 4 + child * 4, 0x80000000U | (child == 7 ? 40 : 32));
    put16(tree, 36, 0x1234); put16(tree, 38, 1); put16(tree, 40, 2);
    put16(tree, 44, 0x5678); put16(tree, 46, 2); put16(tree, 48, 1);
    auto bytes = kcl(tree);
    put_vector(bytes, 20, {0, 0, 0});
    put32(bytes, 32, 0xfffffff8U); put32(bytes, 36, 0xfffffff8U); put32(bytes, 40, 0xfffffff8U);
    put32(bytes, 44, 3);
    put_float(bytes, 132, 0.5f);
    return bytes;
}
Fxyz point(float x, float y, float z) {
    Fxyz out; out.x=x; out.y=y; out.z=z; return out;
}
void point_queries(KCollisionServer& server) {
    auto query = [&](Fxyz position, float scale, int index, float expectedDepth) {
        float depth = -99;
        auto* prism = server.checkPoint(&position, scale, &depth);
        require(prism == (index < 0 ? nullptr : server.getPrismData(index)), "Point query returned the wrong original prism identity");
        require(index < 0 ? depth == -99 : std::abs(depth-expectedDepth)<0.00001f,
                "Point depth or miss-output preservation differs");
    };
    query(point(3.25f, 4.25f, 4.75f), 1, 0, 0.25f);
    query(point(4, 4, 4.75f), 1, 1, 0.25f); // Other octant stores the reversed list.
    query(point(3, 4, 5), 1, 0, 0);
    query(point(3.5f, 4.5f, 4.25f), 1, 0, 0.75f);
    query(point(3.5f, 4.5f, 4.249f), 1, -1, 0);
    query(point(3.5f, 4.5f, 5.001f), 1, -1, 0);
    query(point(2.999f, 4.5f, 4.75f), 1, -1, 0);
    query(point(4.5f, 5.5f, 4.75f), 1, -1, 0);
    query(point(4, 5, 4.75f), 1, 0, 0.25f);
    query(point(9, 4, 4.75f), 1, -1, 0);
    query(point(3.25f, 4.25f, 4.1f), 2, 0, 0.9f);
    query(point(3.25f, 4.25f, 4.5f), 0.5f, -1, 0);
    server.getPrismData(0)->mHeight = -1;
    query(point(3.25f, 4.25f, 4.75f), 1, 1, 0.25f);
    server.getPrismData(1)->mHeight = 0;
    query(point(3.25f, 4.25f, 4.75f), 1, -1, 0);
    server.getPrismData(0)->mHeight = 1;
    server.getPrismData(1)->mHeight = 0.5f;
    std::cout << "PASS original point leaf order, masks, edge/depth boundaries, scaled thickness and disabled prisms\n";
}
void area_queries(KCollisionServer& server) {
    auto query = [&](Fxyz a, Fxyz b, u32 capacity, std::initializer_list<int> expected) {
        std::array<KC_PrismData*, 4> hits{};
        auto count=server.checkArea3D(&a,&b,hits.data(),capacity);
        require(count==expected.size(), "Area query returned the wrong bounded count");
        std::size_t i=0;
        for (int index:expected) require(hits[i++]==server.getPrismData(index), "Area query changed original prism order/identity");
        for (;i<hits.size();++i) require(hits[i]==nullptr, "Area query wrote past its returned bounded result");
    };
    query(point(0,0,0),point(7,7,7),4,{0,1}); // Repeated leaf/prism identities appear across octants.
    query(point(7,7,7),point(0,0,0),4,{0,1});
    query(point(0,0,0),point(7,7,7),1,{0});
    query(point(3.25f,4.25f,5),point(3.25f,4.25f,5),4,{0,1});
    query(point(4.75f,4.25f,4.9f),point(4.9f,4.5f,5.1f),4,{0});
    query(point(5,4.25f,4.9f),point(5.2f,4.5f,5.1f),4,{0});
    query(point(5.01f,4.25f,4.9f),point(5.2f,4.5f,5.1f),4,{});
    query(point(4.75f,5.75f,4.9f),point(4.9f,5.9f,5.1f),4,{0}); // Original candidate test uses triangle boxes.
    query(point(9,9,9),point(10,10,10),4,{});
    server.getPrismData(0)->mHeight = -1;
    query(point(0,0,0),point(7,7,7),4,{1});
    server.getPrismData(0)->mHeight = 1;
    std::cout << "PASS original area octree traversal, duplicate suppression, capacity, zero extents and inclusive boxes\n";
}
}

int main() {
    if (original_resource_fixture_main()!=0) return 1;
    using namespace smgpc;
    auto runtime=compat::JkrHeapRuntime::create(8U<<20);
    auto domain=compat::JkrAllocationDomain::create(runtime,2U<<20);
    auto bytes=std::make_shared<Bytes>(query_resource());
    auto paBytes=std::make_shared<Bytes>(attributes());
    const auto original=*bytes;
    auto alias=std::make_unique<resource::KCollisionSourceRegistration>(*bytes,bytes);
    auto pa=resource::register_jmap_source(*paBytes,paBytes);
    KCollisionServer* first;
    KCollisionServer* second;
    {
        compat::JkrAllocationScope heap(domain);
        first=new KCollisionServer(); second=new KCollisionServer();
        first->init(bytes->data(),paBytes->data()); second->init(bytes->data(),paBytes->data());
    }
    require(JKRHeap::findFromRoot(first)==&domain->heap() && JKRHeap::findFromRoot(first->mapInfo)==&domain->heap(),
            "Queries must use real original server/JMap allocations in the scene heap");
    require(first->mFile==second->mFile,"Raw alias must preserve shared original collision-resource identity");
    point_queries(*first); area_queries(*first);
    first->getPrismData(0)->mHeight=-1;
    auto p=point(3.25f,4.25f,4.75f);float depth;
    require(second->checkPoint(&p,1,&depth)==second->getPrismData(1),"Second server must observe original shared prism initialization state");
    first->getPrismData(0)->mHeight=1;
    require(*bytes==original,"Actual query/typed initialization must not mutate archived bytes");
    JMapInfo retained=*first->mapInfo;
    std::weak_ptr<Bytes> lifetime=bytes;bytes.reset();
    domain.reset(); // Actual JKR disposers destroy the original server map attachments.
    u32 value=0;
    require(retained.getValue(0,"Floor_code",&value) && value==7,"PA alias must survive heap disposal through its retained owner");
    alias.reset();require(lifetime.expired(),"Raw collision source should retire after final owner release");
    std::cout << "PASS actual heap servers, shared raw resource identity, immutable archive and disposer teardown\n";
}
