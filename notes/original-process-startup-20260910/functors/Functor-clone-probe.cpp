#include "Game/Util/Functor.hpp"
class GameSystemStationedArchiveLoader;
class ResourceHolderManager;
class CreateResourceHolderArgs;
template class MR::FunctorV1M< GameSystemStationedArchiveLoader*, void (GameSystemStationedArchiveLoader::*)(bool), bool >;
template class MR::FunctorV2M< ResourceHolderManager*, void (ResourceHolderManager::*)(const char*, CreateResourceHolderArgs*), const char*, CreateResourceHolderArgs* >;
