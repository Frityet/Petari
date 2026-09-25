#include "Game/Player/GroupChecker.hpp"
#include "Game/Util/HashUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/Cp932Literal.hpp"
#include <memory>

GroupChecker::GroupChecker(const char* pName, u32 a2) : NameObj(pName) {
    mHashTable = new HashSortTable(a2);
}

void GroupChecker::initAfterPlacement() {
    mHashTable->sort();
}

void GroupChecker::add(const NameObj* pObj) {
    const char* name = pObj->mName;
    u32 hashCode = MR::getHashCode(name);
    mHashTable->add(name, 0, true);
}

void GroupCheckManager::add(const NameObj* object, s32 index) {
    mGroups[index]->add(object);
}

bool GroupCheckManager::isExist(const NameObj* object, s32 index) {
    return mGroups[index]->mHashTable->search(object->mName, nullptr);
}

GroupChecker::~GroupChecker() {
    delete mHashTable;
}

GroupCheckManager::~GroupCheckManager() {
    delete mGroups[1];
    delete mGroups[0];
}

GroupCheckManager::GroupCheckManager(const char* pName) : NameObj(pName), mGroups{}, _14(2) {
    // Register each child with its actual owner; construction can unwind before
    // the manager is complete, so retain local ownership until both exist.
    auto shell = std::make_unique<GroupChecker>(CP932("カメサーチ対象物グループ"), 0x20);
    smgpc::compat::claim_name_obj_runtime_ownership(shell.get(), this);
    auto spinning = std::make_unique<GroupChecker>(CP932("スピニングボックス反射グループ"), 0x8);
    smgpc::compat::claim_name_obj_runtime_ownership(spinning.get(), this);
    mGroups[0] = shell.release();
    mGroups[1] = spinning.release();
}
