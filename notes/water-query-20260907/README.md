# Original water query recovery

The new `MR::getWaterAreaObj` body was recovered from the retail 112-byte PowerPC function, following `decomp/AGENT_DECOMP_GUIDE.md`, then mirrored into the native source. It clears all output state, queries the real Water AreaObj manager, returns that matched area first, and otherwise delegates to the original ocean holder. No replacement absent-water result was added.

## Retail validation

All seven original translation units compile with the recorded MW GC 3.0a3 toolchain. Full command arrays, source hashes, logs, and objdiff reports are in this directory.

| Method | Match |
| --- | ---: |
| `MR::getWaterAreaObj(WaterInfo*, const JGeometry::TVec3<float>&)` | 99.46429% |
| `OceanBowl::isInWater(const JGeometry::TVec3<float>&) const` | 100.0% |
| `OceanRing::isInWater(const JGeometry::TVec3<float>&) const` | 99.91803% |
| `OceanRing::calcNearestPos(const JGeometry::TVec3<float>&, JGeometry::TVec3<float>*, JGeometry::TVec3<float>*, JGeometry::TVec3<float>*) const` | 99.74719% |
| `OceanRing::calcCurrentWidthRate(float) const` | 99.791664% |
| `OceanRing::getPoint(int, int) const` | 100.0% |
| `OceanSphere::isInWater(const JGeometry::TVec3<float>&) const` | 100.0% |
| `OceanSpherePlane::getPoint(int, int) const` | 100.0% |
| `WaterAreaFunction::tryInOceanArea(const JGeometry::TVec3<float>&, WaterInfo*)` | 100.0% |
| `WaterInfo::WaterInfo()` | 100.0% |
| `WaterInfo::isInWater() const` | 100.0% |
| `WaterInfo::clear()` | 100.0% |

## Native closure inventory

The existing original membership rules are: bowl radius plus its local up-plane half-space; sphere alive state and radius; ring coarse box, nearest original sampled centerline, per-rail-point width interpolation, then the actual scene gravity half-space. The holder checks bowls, then rings, then spheres in their original insertion order and returns the first hit. An absent holder returns false in the original source.

Full original OceanRing and OceanSphere translation units compiled in an isolated notes-only inventory with native headers first and original headers only as a fallback for declarations absent from the port. WaterInfo compiled too. OceanBowl requires the missing DrawUtil `loadTexProjectionMtx(u32)` declaration and complete Color8 in the native utility umbrella. WaterAreaHolder requires a 64-bit layout fix to its included incomplete WhirlPoolAccelerator header (the original total-size-minus-native-base padding underflows). These inventory commands do not change root Xmake state.

The ring query links through these additional original helpers: `calcRailDirectionAtCoord`, `calcRailPosAndDirectionAtCoord`, `getRailDirection`, `calcDistanceToCurrentAndNextRailPoint`, float overloads of `getCurrentRailPointArg1NoInit` and `getNextRailPointArg1NoInit`, their original float argument converters, and the original unclamped `calcPerpendicFootToLine`. Existing native RailRider operations provide actual coordinates, direction and point arguments; the existing inside-line helper already preserves the original arithmetic.

Runtime activation remains a separate boundary: native AreaObj descriptors currently do not install WaterArea/WaterAreaMgr, and the native scene-object factories do not create WaterAreaHolder yet. Their constructors also create the original WaterCameraFilter and screen bloom machinery. This recovery does not replace those missing systems with empty water data, and its native runtime availability is not claimed yet.

The completed native cohort imports whole original WaterInfo, WaterAreaHolder, OceanBowl, OceanRing and OceanSphere translation units and their absent declarations. All seven affected translation units (including AreaObjUtil and GameRailCompat) compile with the actual native LLVM 23 command and native-only headers; see native-compile-results.json. There are 19 source/header imports, 18 byte-identical; WhirlPoolAccelerator alone preserves its original 0x38-byte derived tail independently of host pointer width. The six public RailUtil methods and three private helper bodies were copied from the original source into the existing native rail provider. The newly recovered getWaterAreaObj body is identical in decomp and native AreaObjUtil, preserving unrelated existing native methods. No Xmake configuration was modified, no root build was run by this recovery, and no gameplay execution is claimed. Parent owns the remaining original unclamped line-foot math helper and coordinated final linking.

The copied rail helper cohort was also rebuilt with the Wii compiler. Eight of the nine imported bodies match 100%; `calcDistanceToCurrentAndNextRailPoint` matches 93.541664%, preserving the original source's documented register scheduling/reload mismatch. See rail-wii-query-proof.json; the full RailUtil report also contains the unchanged signed-integer overloads, which this native cohort does not import.
