# Wii per-symbol comparison

Scores are objdiff percentages against the original retail object.
Baseline means the restored preflatten source compiled with the same current headers and flags.

| Translation unit | Symbol | Retail bytes | Current % | Baseline % |
| --- | --- | ---: | ---: | ---: |
| AutoEffectInfo | `bool JMapInfoIter::getValue<const char*>(const char*, const char**) const` | 100 | 100.000000 | 100.000000 |
| AutoEffectInfo | `@unnamed@AutoEffectInfo_cpp@::str2Color(const char*)` | 48 | 100.000000 | 100.000000 |
| AutoEffectInfo | `@unnamed@AutoEffectInfo_cpp@::isValueOnR(const JMapInfoIter&, const char*)` | 64 | 99.687500 | 99.687500 |
| AutoEffectInfo | `@unnamed@AutoEffectInfo_cpp@::isValueOnS(const JMapInfoIter&, const char*)` | 64 | 99.687500 | 99.687500 |
| AutoEffectInfo | `@unnamed@AutoEffectInfo_cpp@::getStringValue(const JMapInfoIter&, const char*)` | 76 | 99.473690 | 99.473690 |
| AutoEffectInfo | `AutoEffectInfo::AutoEffectInfo()` | 92 | 100.000000 | 100.000000 |
| AutoEffectInfo | `AutoEffectInfo::init(const JMapInfoIter&)` | 960 | 99.191666 | 99.191666 |
| AutoEffectInfo | `AutoEffectInfo::getName() const` | 28 | 100.000000 | 100.000000 |
| AutoEffectInfo | `@unnamed@AutoEffectInfo_cpp@::isValueOnT(const JMapInfoIter&, const char*)` | 64 | 99.687500 | 99.687500 |
| AutoEffectInfo | `Color8::set(_GXColor)` | 36 | 100.000000 | 100.000000 |
| AutoEffectGroup | `AutoEffectGroup::AutoEffectGroup(const char*, int)` | 92 | 100.000000 | 100.000000 |
| AutoEffectGroup | `AutoEffectGroup::add(const JMapInfoIter&)` | 116 | 100.000000 | 100.000000 |
| AutoEffectGroup | `MR::Effect::addAutoEffectsFromGroup(const AutoEffectGroup*, EffectKeeper*, const LiveActor*)` | 108 | 100.000000 | 100.000000 |
| AutoEffectGroup | `MR::Effect::addAutoEffectsFromGroup(const AutoEffectGroup*, PaneEffectKeeper*, const LayoutActor*)` | 108 | 100.000000 | 100.000000 |
| AutoEffectGroup | `MR::Effect::addAutoEffectsFromGroup(const AutoEffectGroup*, MultiSceneEffectKeeper*, const MultiSceneActor*)` | 108 | 100.000000 | 100.000000 |
| AutoEffectGroup | `MR::Effect::createAutoEffectGroup(const char*)` | 412 | 99.805824 | 99.805824 |
| AutoEffectGroupHolder | `AutoEffectGroupHolder::AutoEffectGroupHolder()` | 12 | 100.000000 | 100.000000 |
| AutoEffectGroupHolder | `AutoEffectGroupHolder::find(const char*) const` | 96 | 100.000000 | 100.000000 |
| AutoEffectGroupHolder | `AutoEffectGroupHolder::isExist(const char*) const` | 88 | 100.000000 | 100.000000 |
| AutoEffectGroupHolder | `MR::Effect::createAndAddAutoEffectGroup(AutoEffectGroupHolder*, const char*)` | 120 | 100.000000 | 100.000000 |
| AutoEffectGroupHolder | `MR::Effect::registerAutoEffectInfos(AutoEffectGroupHolder*, EffectKeeper*, const LiveActor*, const char*)` | 80 | 100.000000 | 100.000000 |
| AutoEffectGroupHolder | `MR::Effect::registerAutoEffectInfos(AutoEffectGroupHolder*, PaneEffectKeeper*, const LayoutActor*, const char*)` | 80 | 100.000000 | 100.000000 |
| AutoEffectGroupHolder | `MR::Effect::registerAutoEffectInfos(AutoEffectGroupHolder*, MultiSceneEffectKeeper*, const MultiSceneActor*, const char*)` | 80 | 100.000000 | 100.000000 |
| AutoEffectGroupHolder | `AutoEffectGroup* const* std::find_if<AutoEffectGroup* const*, MR::eq_ptr_case<AutoEffectGroup*>>(AutoEffectGroup* const*, AutoEffectGroup* const*, MR::eq_ptr_case<AutoEffectGroup*>)` | 104 | 100.000000 | 100.000000 |
| EffectSystemUtil | `@unnamed@EffectSystemUtil_cpp@::setupMultiEmitter(MultiEmitter*, const AutoEffectInfo*)` | 276 | 99.782610 | 99.782610 |
| EffectSystemUtil | `MR::Effect::requestMovementOnAllEmitters()` | 52 | 100.000000 | absent |
| EffectSystemUtil | `MR::Effect::requestMovementOffAllLoopEmitters()` | 40 | 100.000000 | absent |
| EffectSystemUtil | `MR::Effect::checkEffectSceneUpdate(const EffectSystem*)` | 8 | 100.000000 | absent |
| EffectSystemUtil | `MR::Effect::movementEffectNormal(const EffectSystem*)` | 8 | 100.000000 | absent |
| EffectSystemUtil | `MR::Effect::drawEffect3D(const EffectSystem*, const JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<float>>>&)` | 8 | 100.000000 | absent |
| EffectSystemUtil | `MR::Effect::drawEffect2D(const EffectSystem*)` | 8 | 100.000000 | absent |
| EffectSystemUtil | `MR::Effect::forceDeleteAllEmitters(const EffectSystem*)` | 8 | 100.000000 | absent |
| EffectSystemUtil | `MR::Effect::isExistInResource(unsigned short*, const char*)` | 68 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::isEffect2D(const MultiEmitter*)` | 52 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::initEffectSyncBck(EffectKeeper*, const ModelManager*, const char*, const char*, long, float, float, bool)` | 8 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::addEffectSyncBck(MultiEmitter*, const ModelManager*, const char*)` | 8 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::getAutoEffectNum(const char*)` | 52 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::getAutoEffectListBinary()` | 36 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::setupMultiEmitter(EffectKeeper*, const ModelManager*, const AutoEffectInfo*)` | 152 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::setupMultiEmitterSyncBck(EffectKeeper*, const ModelManager*, const AutoEffectInfo*)` | 576 | 95.312500 | 95.312500 |
| EffectSystemUtil | `MR::Effect::registerAutoEffectInfoGroup(EffectKeeper*, const LiveActor*, const char*)` | 96 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::requestMovementOn(EffectKeeper*)` | 120 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::registerAutoEffectInfoGroup(PaneEffectKeeper*, const LayoutActor*, const char*)` | 96 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::registerAutoEffectInfoGroup(PaneEffectKeeper*, const EffectSystem*, const LayoutActor*, const char*)` | 92 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::addAutoEffect(EffectKeeper*, const LiveActor*, const AutoEffectInfo*)` | 240 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::addAutoEffect(PaneEffectKeeper*, const LayoutActor*, const AutoEffectInfo*)` | 88 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::addAutoEffect(MultiSceneEffectKeeper*, const MultiSceneActor*, const AutoEffectInfo*)` | 152 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::registerAutoEffectInfoGroup(MultiSceneEffectKeeper*, const EffectSystem*, const MultiSceneActor*, const char*)` | 92 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::deleteParticleEmitter(ParticleEmitter*)` | 52 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::setLinkSingleEmitter(ParticleEmitter*, SingleEmitter*)` | 12 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::getLinkSingleEmitter(const JPABaseEmitter*)` | 8 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::forceDeleteAllOneTimeEmitter()` | 40 | 100.000000 | absent |
| EffectSystemUtil | `MR::Effect::createParticleEmitter(ParticleEmitter*, JPAEmitterManager*, const JGeometry::TVec3<float>&, unsigned short, unsigned char, unsigned char)` | 108 | 100.000000 | 100.000000 |
| EffectSystemUtil | `MR::Effect::isExistInResource(unsigned short*, const char*, long)` | 88 | 99.545456 | 99.545456 |
| EffectSystemUtil | `MR::Effect::getEffectAttributeName(long)` | 92 | 98.043480 | absent |
| EffectSystemUtil | `MR::Effect::createAutoEffect(const char*, const char*)` | 308 | 93.571430 | absent |
| EffectUtil | `@unnamed@EffectUtil_cpp@::isExistEffect(const LiveActor*, const char*)` | 56 | 100.000000 | 100.000000 |
| EffectUtil | `MR::requestEffectStopSceneStart()` | 4 | 100.000000 | 100.000000 |
| EffectUtil | `MR::requestEffectStopSceneEnd()` | 4 | 100.000000 | 100.000000 |
| EffectUtil | `MR::addEffect(LiveActor*, const char*)` | 12 | 100.000000 | 100.000000 |
| EffectUtil | `MR::getEffect(const LiveActor*, const char*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::getEffect(const LayoutActor*, const char*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::isExistEffectKeeper(const LiveActor*)` | 16 | 100.000000 | 100.000000 |
| EffectUtil | `MR::isExistEffectKeeper(const LayoutActor*)` | 16 | 100.000000 | 100.000000 |
| EffectUtil | `MR::emitEffect(LiveActor*, const char*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::emitEffectWithScale(LiveActor*, const char*, float, long)` | 100 | 100.000000 | 100.000000 |
| EffectUtil | `MR::emitEffectWithEmitterCallBack(LiveActor*, const char*, MultiEmitterCallBackBase*)` | 72 | 100.000000 | 100.000000 |
| EffectUtil | `MR::emitEffectWithParticleCallBack(LiveActor*, const char*, MultiEmitterParticleCallBack*)` | 76 | 100.000000 | 100.000000 |
| EffectUtil | `MR::tryEmitEffect(LiveActor*, const char*)` | 84 | 100.000000 | 100.000000 |
| EffectUtil | `MR::tryDeleteEffect(LiveActor*, const char*)` | 88 | 100.000000 | 100.000000 |
| EffectUtil | `MR::deleteEffect(LiveActor*, const char*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::forceDeleteEffect(LiveActor*, const char*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::deleteEffectAll(LiveActor*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::forceDeleteEffectAll(LiveActor*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::isRegisteredEffect(const LiveActor*, const char*)` | 32 | 100.000000 | 100.000000 |
| EffectUtil | `MR::isEffectValid(const LiveActor*, const char*)` | 56 | 100.000000 | 100.000000 |
| EffectUtil | `MR::onDrawEffect(LiveActor*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::offDrawEffect(LiveActor*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::pauseOffEffectAll(LiveActor*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::onEmitEffectSyncClipping(LiveActor*, const char*)` | 40 | 100.000000 | 100.000000 |
| EffectUtil | `MR::onForceDeleteEffectSyncClipping(LiveActor*, const char*)` | 40 | 100.000000 | 100.000000 |
| EffectUtil | `MR::setEffectName(LiveActor*, const char*, const char*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::setEffectHostSRT(LiveActor*, const char*, const JGeometry::TVec3<float>*, const JGeometry::TVec3<float>*, const JGeometry::TVec3<float>*)` | 80 | 100.000000 | 100.000000 |
| EffectUtil | `MR::setEffectHostMtx(LiveActor*, const char*, float(*)[4])` | 56 | 100.000000 | 100.000000 |
| EffectUtil | `MR::setEffectBaseScale(LiveActor*, const char*, float)` | 56 | 100.000000 | 100.000000 |
| EffectUtil | `MR::setEffectLocalScale(LiveActor*, const char*, const JGeometry::TVec3<float>&)` | 60 | 100.000000 | 100.000000 |
| EffectUtil | `MR::setEffectColor(LiveActor*, const char*, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char)` | 136 | 100.000000 | 100.000000 |
| EffectUtil | `MR::setEffectPrmColor(LiveActor*, const char*, unsigned char, unsigned char, unsigned char)` | 84 | 100.000000 | 100.000000 |
| EffectUtil | `MR::setEffectEnvColor(LiveActor*, const char*, unsigned char, unsigned char, unsigned char)` | 84 | 100.000000 | 100.000000 |
| EffectUtil | `MR::emitEffectHit(LiveActor*, const JGeometry::TVec3<float>&, const char*)` | 80 | 100.000000 | 100.000000 |
| EffectUtil | `MR::emitEffectHit(LiveActor*, const JGeometry::TVec3<float>&, const JGeometry::TVec3<float>&, const char*)` | 112 | 100.000000 | 100.000000 |
| EffectUtil | `MR::emitEffectHit(LiveActor*, float(*)[4], const char*)` | 80 | 100.000000 | 100.000000 |
| EffectUtil | `MR::emitEffectHitBetweenSensors(LiveActor*, const HitSensor*, const HitSensor*, float, const char*)` | 76 | 100.000000 | 100.000000 |
| EffectUtil | `MR::initEffectAfterPlacement(LiveActor*)` | 68 | 100.000000 | 100.000000 |
| EffectUtil | `MR::updateEffectFloorCode(LiveActor*, const Triangle*)` | 8 | 100.000000 | 100.000000 |
| EffectUtil | `MR::updateEffectFloorCodeLineToMap(LiveActor*, const JGeometry::TVec3<float>&, const JGeometry::TVec3<float>&)` | 120 | 100.000000 | 100.000000 |
| EffectUtil | `MR::updateEffectFloorCodeLineToMap(LiveActor*, float, float)` | 196 | 100.000000 | 100.000000 |
| EffectUtil | `MR::initEffectSyncBck(LiveActor*, const char*, const char* const*)` | 172 | 96.930230 | 96.930230 |
| EffectUtil | `MR::addEffectHitNormal(LiveActor*, const char*)` | 100 | 72.400000 | 72.400000 |
| HashUtil | `HashSortTable::HashSortTable(unsigned long)` | 124 | 100.000000 | 100.000000 |
| HashUtil | `HashSortTable::add(const char*, unsigned long, bool)` | 104 | 100.000000 | 100.000000 |
| HashUtil | `HashSortTable::add(unsigned long, unsigned long)` | 52 | 100.000000 | 100.000000 |
| HashUtil | `HashSortTable::addOrSkip(unsigned long, unsigned long)` | 100 | 100.000000 | 100.000000 |
| HashUtil | `HashSortTable::sort()` | 428 | 100.000000 | 100.000000 |
| HashUtil | `HashSortTable::search(unsigned long, unsigned long*)` | 164 | 100.000000 | 100.000000 |
| HashUtil | `HashSortTable::search(const char*, unsigned long*)` | 76 | 100.000000 | 100.000000 |
| HashUtil | `HashSortTable::search(const char*, const char*, unsigned long*)` | 92 | 100.000000 | 100.000000 |
| HashUtil | `HashSortTable::swap(const char*, const char*)` | 136 | 100.000000 | 100.000000 |
| HashUtil | `MR::getHashCode(const char*)` | 44 | 100.000000 | 100.000000 |
| HashUtil | `MR::getHashCodeLower(const char*)` | 96 | 100.000000 | 100.000000 |
