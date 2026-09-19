#include "compat/Cp932Literal.hpp"
#include "Game/MapObj/StageEffectDataTable.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util.hpp"

namespace {
    enum CameraShakeType { None = 0, Strong = 1, Normal = 2, NormalWeak = 3, Weak = 4, VeryWeak = 5 };

    struct StageEffectCameraData {
        const char* objectName;
        CameraShakeType startShake;
        f32 movingIntensity;
        f32 movingSpeed;
        CameraShakeType stopShake;
        f32 distance;
    };

    struct StageEffectPadData {
        const char* objectName;
        const char* startPattern;
        const char* movingPattern;
        const char* stopPattern;
    };

    static StageEffectCameraData sStageEffectCameraDataTable[] = {
        {"LavaHomeVolcanoInnerFlow", None, 0.3f, 2.2f, None, 0.0f},
        {"LavaRotatePlanetStartStep", None, 0.0f, 0.0f, None, 0.0f},
        {"OceanRingRuinsMove", NormalWeak, 0.0f, 0.0f, NormalWeak, 3e+03f},
        {"OceanRingRuinsGearBig", None, 0.0f, 0.0f, None, 0.0f},
        {"OceanRingRuinsGearSmall", None, 0.0f, 0.0f, None, 0.0f},
        {"OceanRotateBridge", None, 0.0f, 0.0f, None, 3e+03f},
        {"ShutterDoorA", None, 0.0f, 0.0f, Normal, 3e+03f},
        {"ShutterDoorD", None, 0.0f, 0.0f, Normal, 3e+03f},
        {"OceanSwingBoard", None, 0.0f, 0.0f, None, 0.0f},
        {"RotateNutStep", None, 0.0f, 0.0f, None, 0.0f},
        {"OnimasuPlanetRailMovePartsA", None, 0.0f, 0.0f, None, 0.0f},
        {"GhostShipCaveMoveGroundA", None, 0.0f, 0.0f, None, 0.0f},
        {"GhostShipCaveMoveGroundB", None, 0.0f, 0.0f, None, 0.0f},
        {"WaterRoadCaveStepA", None, 0.0f, 0.0f, None, 0.0f},
        {"TeresaRoomDoor", None, 0.1f, 2.2f, None, 0.0f},
        {"LavaObstacleRockHomeA", None, 0.1f, 2.2f, None, 0.0f},
        {"LavaObstacleRockShell", None, 0.1f, 2.2f, None, 0.0f},
        {"ForestObstacleWoodA", None, 0.1f, 2.2f, None, 0.0f},
        {"MiniMechaKoopaPartsMoveStepA", None, 0.0f, 0.0f, Normal, 1e+04f},
        {"MiniMechaKoopaPartsCage", None, 0.0f, 0.0f, Normal, 1e+04f},
        {"RayGunPlanetPartsScrew", NormalWeak, 0.0f, 0.0f, Normal, 1e+04f},
        {"KoopaVS1PartsMoveStepA", Weak, 0.0f, 0.0f, Weak, -1.0f},
        {"KoopaVS1PartsMoveStepB", NormalWeak, 0.0f, 0.0f, NormalWeak, -1.0f},
        {"KoopaVS1PartsRotateMoveStepA", Weak, 0.0f, 0.0f, Weak, -1.0f},
        {"KoopaVS1PartsRotateMoveStepB", Weak, 0.0f, 0.0f, Weak, -1.0f},
        {"BeeWallClimbPartsA", Weak, 0.0f, 0.0f, Weak, -1.0f},
        {"KoopaJrSmallShipA", Normal, 0.0f, 0.0f, Normal, 3e+03f},
        {"IceVolcanoAppearStepA", None, 0.0f, 0.0f, Strong, 3e+03f},
        {"IceVolcanoClimbingWall", None, 0.0f, 0.0f, Strong, 1e+04f},
        {"BroadBeanMoveStepA", None, 0.0f, 0.0f, VeryWeak, -1.0f},
        {"BroadBeanMoveStepB", None, 0.0f, 0.0f, VeryWeak, -1.0f},
        {"HeavensDoorInsideCage", None, 0.15f, 1.2f, None, 1e+04f},
    };

    static StageEffectPadData sStageEffectPadDataTable[] = {
        {"LavaHomeVolcanoInnerFlow", CP932("中"), CP932("中"), nullptr},
        {"LavaRotatePlanetStartStep", nullptr, nullptr, nullptr},
        {"OceanRingRuinsMove", nullptr, nullptr, CP932("中")},
        {"OceanRingRuinsGearBig", nullptr, nullptr, nullptr},
        {"OceanRingRuinsGearSmall", nullptr, nullptr, nullptr},
        {"OceanRotateBridge", nullptr, nullptr, CP932("中")},
        {"ShutterDoorA", nullptr, nullptr, CP932("中")},
        {"ShutterDoorD", nullptr, CP932("微弱"), CP932("中")},
        {"OceanSwingBoard", nullptr, nullptr, nullptr},
        {"RotateNutStep", nullptr, nullptr, nullptr},
        {"OnimasuPlanetRailMovePartsA", nullptr, nullptr, nullptr},
        {"GhostShipCaveMoveGroundA", nullptr, nullptr, nullptr},
        {"GhostShipCaveMoveGroundB", nullptr, nullptr, nullptr},
        {"WaterRoadCaveStepA", nullptr, nullptr, nullptr},
        {"TeresaRoomDoor", CP932("中"), nullptr, nullptr},
        {"MiniMechaKoopaPartsMoveStepA", nullptr, nullptr, CP932("中")},
        {"MiniMechaKoopaPartsCage", nullptr, nullptr, CP932("中")},
        {"RayGunPlanetPartsScrew", CP932("中"), CP932("微弱"), CP932("中")},
        {"KoopaVS1PartsMoveStepA", CP932("弱"), nullptr, CP932("弱")},
        {"KoopaVS1PartsMoveStepB", CP932("弱"), nullptr, CP932("弱")},
        {"KoopaVS1PartsRotateMoveStepA", CP932("弱"), nullptr, CP932("弱")},
        {"KoopaVS1PartsRotateMoveStepB", CP932("弱"), nullptr, CP932("弱")},
        {"BeeWallClimbPartsA", CP932("弱"), nullptr, CP932("弱")},
        {"KoopaJrSmallShipA", CP932("中"), nullptr, CP932("強")},
        {"IceVolcanoAppearStepA", nullptr, CP932("弱"), CP932("強")},
        {"IceVolcanoClimbingWall", nullptr, CP932("弱"), CP932("強")},
        {"BroadBeanMoveStepA", nullptr, nullptr, CP932("弱")},
        {"BroadBeanMoveStepB", nullptr, nullptr, CP932("弱")},
        {"HeavensDoorInsideCage", nullptr, CP932("弱"), nullptr},
    };

    static StageEffectSoundData sStageEffectSeDataTable[] = {
        {"LavaHomeVolcanoInnerFlow", 0, "SE_OJ_LV_LAVA_INCREASE_HOME", 0, 0xffffffff, 0},
        {"LavaRotatePlanetStartStep", "SE_OJ_LAVA_BR_ROCK_L_ON", "SE_OJ_LV_LAVA_BR_ROCK_L_MV", 0, 0xffffffff, 0},
        {"OceanRingRuinsMove", "SE_OJ_OCN_RUIN_MOVE_START", "SE_OJ_LV_OCN_RUIN_MOVE", "SE_OJ_OCN_RUIN_STOP", 0xffffffff, 0},
        {"OceanRingRuinsGearBig", "SE_OJ_OCN_RUIN_GEAR_L_ST", "SE_OJ_LV_OCN_RUIN_MOVE", "SE_OJ_OCN_RUIN_GEAR_L_END", 0xffffffff, 0},
        {"OceanRingRuinsGearSmall", 0, "SE_OJ_LV_OCN_RUIN_GEAR_S_MV", 0, 0xffffffff, 0},
        {"OceanRotateBridge", "SE_OJ_O_ROTATE_BRIDGE_ST", "SE_OJ_LV_O_ROTATE_BRIDGE_MV", "SE_OJ_O_ROTATE_BRIDGE_END", 0xffffffff, 2},
        {"ShutterDoorA", 0, "SE_OJ_LV_SHUTTER_DOOR_MOVE", "SE_OJ_SHUTTER_DOOR_STOP", 0xffffffff, 0},
        {"OceanSwingBoard", "SE_OJ_O_SWG_BOARD_MOVE_ST", "SE_OJ_LV_O_SWG_BOARD_MOVE", 0, 0xffffffff, 0},
        {"RotateNutStep", 0, "SE_OJ_LV_ROTATE_NUT_MOVE", 0, 0xffffffff, 0},
        {"OceanFloaterTowerRotateStepA", "SE_OJ_O_TOWER_ROTATE_START", "SE_OJ_LV_O_TOWER_ROTATE_MV", 0, 0xffffffff, 1},
        {"OceanFloaterTowerRotateStepB", "SE_OJ_O_TOWER_ROTATE_START", "SE_OJ_LV_O_TOWER_ROTATE_MV", 0, 0xffffffff, 1},
        {"OceanFloaterTowerRotateStepC", "SE_OJ_O_TOWER_ROTATE_START", "SE_OJ_LV_O_TOWER_ROTATE_MV", 0, 0xffffffff, 0},
        {"OceanFloaterTowerRotateStepD", "SE_OJ_O_TOWER_ROTATE_START", "SE_OJ_LV_O_TOWER_ROTATE_MV", 0, 0xffffffff, 0},
        {"OceanAppearBridge", "SE_OJ_O_APPEAR_BRDG_APPEAR", 0, 0, 0xffffffff, 0},
        {"OnimasuPlanetRotatePartsA", 0, "SE_OJ_LV_STONE_ROTATOR_MOVE", 0, 0xffffffff, 0},
        {"OnimasuPlanetRotatePartsB", 0, "SE_OJ_LV_STONE_ROTATOR_MOVE", 0, 0xffffffff, 0},
        {"OnimasuPlanetRailMovePartsA", "SE_OJ_STONE_FLOOR_START", "SE_OJ_LV_STONE_FLOOR_MOVE", "SE_OJ_STONE_FLOOR_END", 0xffffffff, 0},
        {"GhostShipCaveMoveGroundA", "SE_OJ_PTM_GROUND_START", "SE_OJ_LV_PTM_GROUND_MOVE", "SE_OJ_PTM_GROUND_STOP", 0xffffffff, 0},
        {"GhostShipCaveMoveGroundB", "SE_OJ_PTM_GROUND_START", "SE_OJ_LV_PTM_GROUND_MOVE", "SE_OJ_PTM_GROUND_STOP", 0xffffffff, 0},
        {"WaterRoadCaveStepA", "SE_OJ_W_RD_CAVE_STEP_ST", "SE_OJ_LV_W_RD_CAVE_STEP_MV", "SE_OJ_W_RD_CAVE_STEP_ED", 0xffffffff, 0},
        {"LavaBreakColumn", "SE_OJ_LAVA_COLUMN_FALL_ST", "SE_OJ_LV_LAVA_COLUMN_FALL", 0, 0x4b, 0},
        {"LavaBreakRockA", "SE_OJ_LAVA_BR_ROCK_H_ON", "SE_OJ_LV_LAVA_BR_ROCK_H_MV", 0, 0xffffffff, 0},
        {"LavaBreakRockB", "SE_OJ_LAVA_BR_ROCK_S_ON", "SE_OJ_LV_LAVA_BR_ROCK_S_MV", 0, 0xffffffff, 0},
        {"LavaBreakRockC", "SE_OJ_LAVA_BR_ROCK_L_ON", "SE_OJ_LV_LAVA_BR_ROCK_L_MV", 0, 0xffffffff, 0},
        {"LavaBreakRockD", "SE_OJ_LAVA_BR_ROCK_S_ON", "SE_OJ_LV_LAVA_BR_ROCK_S_MV", 0, 0xffffffff, 0},
        {"LavaFallDownColumnA", "SE_OJ_LAVA_COLUMN_FALL_1", 0, 0, 0xffffffff, 0},
        {"LavaFallDownColumnB", "SE_OJ_LAVA_COLUMN_FALL_2", 0, 0, 0xffffffff, 0},
        {"OceanHomeTopRock", 0, 0, 0, 0xffffffff, 0},
        {"WaterWheelGear", 0, "SE_OJ_LV_W_WHEEL_GEAR_MOVE", 0, 0xffffffff, 0},
        {"TeresaRoomDoor", "SE_OJ_TERESA_ROOM_DOOR_OPEN", 0, 0, 0xffffffff, 0},
        {"WaterPowerStream", "SE_OJ_W_POWER_STREAM_ST", "SE_OJ_LV_W_POWER_STREAM_MV", 0, 0xffffffff, 0},
        {"LavaObstacleRockShell", "SE_OJ_LAVA_OB_ROCK_S_ON", "SE_OJ_LV_LAVA_OB_ROCK_S_MV", "SE_OJ_LAVA_OB_ROCK_S_STOP", 0x122, 0},
        {"LavaObstacleRockHomeA", "SE_OJ_LAVA_OB_ROCK_A_ON", "SE_OJ_LV_LAVA_OB_ROCK_A_MV", "SE_OJ_LAVA_OB_ROCK_A_STOP", 0xd2, 0},
        {"LavaObstacleRockHomeB", 0, "SE_OJ_LV_LAVA_OB_ROCK_B_MV", "SE_OJ_LAVA_OB_ROCK_B_STOP", 0x118, 0},
        {"UFOKinoko", 0, "SE_OJ_LV_UFO_KINOKO_ROTATE", 0, 0xffffffff, 0},
        {"PhantomShipPropellerSmall", 0, "SE_OJ_LV_P_SHIP_PROPELLER_S", 0, 0xffffffff, 0},
        {"PhantomShipPropellerBig", 0, "SE_OJ_LV_P_SHIP_PROPELLER_L", 0, 0xffffffff, 0},
        {"ForestAppearStepA", "SE_OJ_FOREST_STEP_APPEAR", 0, 0, 0xffffffff, 0},
        {"ForestObstacleWoodA", "SE_OJ_FOREST_OBS_WOOD_ON", "SE_OJ_LV_FOREST_OBS_WOOD_MV", "SE_OJ_FOREST_OBS_WOOD_STOP", 0x78, 2},
        {"RGTowerPressGroundA", "SE_OJ_STONE_FLOOR_S_START", "SE_OJ_LV_STONE_FLOOR_S_MOVE", "SE_OJ_STONE_FLOOR_S_END", 0xffffffff, 0},
        {"TeresaMansionEntranceDoor", 0, 0, "SE_OJ_P_ROOM_DOOR_L_CL_ST", 0xffffffff, 0},
        {"BattleShipMovePartsA", 0, "SE_OJ_LV_METAL_ROTATOR_MOVE", 0, 0xffffffff, 0},
        {"ReverseGravityRoomPressGroundA", "SE_OJ_METAL_FLOOR_S_START", "SE_OJ_LV_METAL_FLOOR_S_MOVE", "SE_OJ_METAL_FLOOR_S_STOP", 0xffffffff, 0},
        {"ReverseGravityRoomPressGroundB", "SE_OJ_METAL_FLOOR_L_START", "SE_OJ_LV_METAL_FLOOR_L_MOVE", "SE_OJ_METAL_FLOOR_L_STOP", 0xffffffff, 0},
        {"ShutterDoorB", "SE_OJ_METAL_FLOOR_L_START", "SE_OJ_LV_METAL_FLOOR_L_MOVE", "SE_OJ_METAL_FLOOR_L_STOP", 0xffffffff, 2},
        {"TeresaMansionHoleCover", 0, 0, "SE_OJ_STRANGEROCK_BREAK_S", 0xffffffff, 0},
        {"KoopaStatue", 0, 0, "SE_OJ_STRANGEROCK_BREAK_S", 0xffffffff, 2},
        {"TeresaRacePartsA", 0, "SE_OJ_LV_TR_RACE_ROTATER", 0, 0xffffffff, 0},
        {"TeresaRacePartsB", "SE_OJ_TR_RACE_PARTS_BOUND", 0, 0, 0xffffffff, 0},
        {"BeeWallClimbPartsA", 0, "SE_OJ_LV_BEE_WALL_MOVE", "SE_OJ_BEE_WALL_STOP", 0xffffffff, 0},
        {"ShutterDoorC", "SE_OJ_FENCE_SHUTTER_START", "SE_OJ_LV_FENCE_SHUTTER_MOVE", "SE_OJ_FENCE_SHUTTER_STOP", 0xae, 2},
        {"ShutterDoorD", "SE_OJ_FENCE_SHUTTER_S_ST", "SE_OJ_LV_FENCE_SHUTTER_S_MV", "SE_OJ_FENCE_SHUTTER_S_ED", 0xb4, 0},
        {"IcicleRock", 0, 0, "SE_OJ_STRANGEROCK_BREAK_S", 0xffffffff, 0},
        {"QuestionBoxMoveStepA", 0, "SE_OJ_LV_Q_BOX_ROTATE", 0, 0xffffffff, 0},
        {"QuestionBoxMoveStepB", 0, "SE_OJ_LV_Q_BOX_RAIL_MOVE", 0, 0xffffffff, 0},
        {"QuestionBoxMoveStepC", 0, "SE_OJ_LV_Q_BOX_RAIL_UD", "SE_OJ_Q_BOX_RAIL_UD_STOP", 0xffffffff, 0},
        {"KoopaVS1PartsMoveStepA", "SE_OJ_KOOPA_F_LIFT_ST", "SE_OJ_LV_KOOPA_F_LIFT_MV", "SE_OJ_KOOPA_F_LIFT_ED", 0xffffffff, 0},
        {"KoopaVS1PartsMoveStepB", "SE_OJ_KOOPA_F_LIFT_ST", "SE_OJ_LV_KOOPA_F_LIFT_MV", "SE_OJ_KOOPA_F_LIFT_ED", 0xffffffff, 0},
        {"KoopaVS1PartsRotateMoveStepA", "SE_OJ_KOOPA_F_R_LIFT_ST", "SE_OJ_LV_KOOPA_F_R_LIFT_MV", "SE_OJ_KOOPA_F_R_LIFT_ED", 0xffffffff, 0},
        {"KoopaVS1PartsRotateMoveStepB", "SE_OJ_KOOPA_F_R_LIFT_ST", "SE_OJ_LV_KOOPA_F_R_LIFT_MV", "SE_OJ_KOOPA_F_R_LIFT_ED", 0xffffffff, 0},
        {"SideSpikeMoveStepA", "SE_OJ_SIDE_SPIKE_STEP_ST", "SE_OJ_LV_SIDE_SPIKE_STEP_MV", "SE_OJ_SIDE_SPIKE_STEP_ED", 0xffffffff, 0},
        {"PhantomTowerMoveStepA", "SE_OJ_STONE_FLOOR_S_START", "SE_OJ_LV_STONE_FLOOR_S_MOVE", "SE_OJ_STONE_FLOOR_S_END", 0xffffffff, 0},
        {"IceVolcanoAppearStepA", "SE_OJ_VOL_APPEAR_STEP_ST", "SE_OJ_LV_VOL_APPEAR_STEP_MV", "SE_OJ_VOL_APPEAR_STEP_ED", 0xda, 2},
        {"IceVolcanoClimbingWall", "SE_OJ_VOL_CLIMB_WALL_ST", "SE_OJ_LV_VOL_CLIMB_WALL_MV", "SE_OJ_VOL_CLIMB_WALL_ED", 0x1ca, 2},
        {"IceUpDownShutterDoorA", "SE_OJ_MTL_SHUTTER_S_ST", "SE_OJ_LV_MTL_SHUTTER_S_MV", "SE_OJ_MTL_SHUTTER_S_ED", 0x90, 2},
        {"IceVolcanoMoveStepA", 0, "SE_OJ_LV_VOL_MOVE_STEP_FLOW", 0, 0xffffffff, 0},
        {"HopperBeltConveyerRotatePartsA", 0, "SE_OJ_LV_HOPPER_CONVEYER_MV", 0, 0xffffffff, 0},
        {"HopperBeltConveyerMovePartsA", 0, "SE_OJ_LV_HOPPER_CONVEYER_MV", 0, 0xffffffff, 0},
        {"MechaKoopaPartsCaterpillar", 0, "SE_OJ_LV_M_KOOPA_CATER_L_MV", 0, 0xffffffff, 0},
        {"BroadBeanMoveStepA", "SE_OJ_STONE_FLOOR_S_START", "SE_OJ_LV_STONE_FLOOR_S_MOVE", "SE_OJ_STONE_FLOOR_S_END", 0xffffffff, 0},
        {"BroadBeanMoveStepB", "SE_OJ_STONE_FLOOR_S_START", "SE_OJ_LV_STONE_FLOOR_S_MOVE", "SE_OJ_STONE_FLOOR_S_END", 0xffffffff, 0},
        {"RayGunPlanetPartsGear", 0, "SE_OJ_LV_RAYGUN_GEAR_ROT", 0, 0xffffffff, 0},
        {"RayGunPlanetPartsScrew", 0, "SE_OJ_LV_RAYGUN_SCREW_ROT", "SE_OJ_RAYGUN_SCREW_STOP", 0xffffffff, 2},
        {"SandStreamHighTowerMoveStepA", "SE_OJ_STONE_FLOOR_S_START", "SE_OJ_LV_STONE_FLOOR_S_MOVE", "SE_OJ_STONE_FLOOR_S_END", 0xffffffff, 0},
        {"MechaKoopaPartsRollerA", 0, "SE_OJ_LV_M_KOOPA_CATER_S_MV", 0, 0xffffffff, 0},
        {"MiniMechaKoopaPartsGear", 0, "SE_OJ_LV_MM_KOOPA_GEAR_ROT", 0, 0xffffffff, 0},
        {"MiniMechaKoopaPartsCage", "SE_OJ_MM_KOOPA_CAGE_START", "SE_OJ_LV_MM_KOOPA_CAGE_ROT", "SE_OJ_MM_KOOPA_CAGE_STOP", 0xffffffff, 2},
        {"MiniMechaKoopaPartsMoveStepA", "SE_OJ_MM_KOOPA_STEP_START", "SE_OJ_LV_MM_KOOPA_STEP_MOVE", "SE_OJ_MM_KOOPA_STEP_STOP", 0xffffffff, 2},
        {"AsteroidBlockRotateStepA", 0, "SE_OJ_LV_AST_BLOCK_ROTATE", 0, 0xffffffff, 0},
        {"SandUpDownTowerBreakableWallA", 0, 0, "SE_OJ_WOOD_WALL_BREAK", 0xffffffff, 0},
        {"SandUpDownTowerBreakableWallB", 0, 0, "SE_OJ_WOOD_WALL_BREAK", 0xffffffff, 0},
        {"HoleDeathSandMoveStepA", "SE_OJ_SAND_MOVE_STEP_ST", "SE_OJ_LV_SAND_MOVE_STEP_MV", "SE_OJ_SAND_MOVE_STEP_ED", 0xffffffff, 0},
        {"CannonFortressMoveWall", "SE_OJ_CNFORT_MOVEWALL_ST", "SE_OJ_LV_CNFORT_MOVEWALL_MV", "SE_OJ_CNFORT_MOVEWALL_ED", 0xffffffff, 0},
        {"MechaKoopaPartsBreast", "SE_OJ_M_KOOPA_BREAST_START", "SE_OJ_LV_M_KOOPA_BREAST_MV", "SE_OJ_M_KOOPA_BREAST_BREAK", 0xffffffff, 1},
        {"ChoConveyorMoveChocoA", 0, "SE_OJ_LV_CHOCONVEYER_MOVE", 0, 0xffffffff, 0},
        {"MechaKoopaPartsCollar", 0, "SE_OJ_LV_M_KOOPA_COLLAR_ROT", 0, 0xffffffff, 0},
        {"MechaKoopaPartsMoveStepA", "SE_OJ_M_KOOPA_MV_STEP_ST", "SE_OJ_LV_M_KOOPA_MV_STEP_MV", "SE_OJ_M_KOOPA_MV_STEP_ED", 0xffffffff, 0},
        {"ChoConveyorPlanetD", 0, "SE_OJ_LV_CHO_CONVEYOR_PLN_D", 0, 0xffffffff, 0},
        {"SeaBottomBigDoorA", "SE_OJ_SEABTM_B_DOOR_START", "SE_OJ_LV_SEABTM_B_DOOR_MOVE", "SE_OJ_SEABTM_B_DOOR_STOP", 0xffffffff, 0},
        {"SeaBottomBigDoorB", "SE_OJ_SEABTM_B_DOOR_START", "SE_OJ_LV_SEABTM_B_DOOR_MOVE", "SE_OJ_SEABTM_B_DOOR_STOP", 0xffffffff, 0},
        {"SeaBottomBigDoorC", "SE_OJ_SEABTM_DOOR_C_START", "SE_OJ_LV_SEABTM_DOOR_C_MOVE", "SE_OJ_SEABTM_DOOR_C_STOP", 0xffffffff, 0},
        {"WaterBazookaTowerMoveStepA", 0, "SE_OJ_LV_W_BZK_TW_STEP_A_MV", "SE_OJ_W_BZK_TW_STEP_A_ED", 0xffffffff, 0},
        {"WaterBazookaTowerMoveStepB", "SE_OJ_W_BZK_TW_STEP_B_ST", "SE_OJ_LV_W_BZK_TW_STEP_B_MV", "SE_OJ_W_BZK_TW_STEP_B_ED", 0xffffffff, 0},
        {"WaterBazookaTowerMoveStepC", "SE_OJ_W_BZK_TW_STEP_C_ST", "SE_OJ_LV_W_BZK_TW_STEP_C_MV", "SE_OJ_W_BZK_TW_STEP_C_ED", 0xffffffff, 0},
        {"KoopaJrSmallShipA", "SE_OJ_KOOPA_JR_S_SHIP_ST", "SE_OJ_LV_KOOPA_JR_S_SHIP_MV", "SE_OJ_KOOPA_JR_S_SHIP_ED", 0xffffffff, 0},
        {"HeavensDoorInsideRotatePartsA", 0, "SE_OJ_LV_HD_INSIDE_ROTATOR", 0, 0xffffffff, 0},
        {"HeavensDoorInsideRotatePartsB", 0, "SE_OJ_LV_HD_INSIDE_ROTATOR", 0, 0xffffffff, 0},
        {"HeavensDoorInsideRotatePartsC", 0, "SE_OJ_LV_HD_INSIDE_ROTATOR", 0, 0xffffffff, 0},
        {"HeavensDoorMiddleRotatePartsA", 0, "SE_OJ_LV_HD_MID_ROTATOR_A", 0, 0xffffffff, 0},
        {"HeavensDoorMiddleRotatePartsB", 0, "SE_OJ_LV_HD_MID_ROTATOR_B", 0, 0xffffffff, 0},
        {"HeavensDoorInsideCage", "SE_OJ_HD_INSIDE_CAGE_BREAK", 0, 0, 0xffffffff, 0},
        {"HeavensDoorInsidePlanetPartsA", "SE_OJ_HD_INSIDE_SUN_OFF", 0, 0, 0xffffffff, 0},
        {"HeavensDoorAppearStepA", "SE_OJ_HD_STEP_APPEAR", 0, 0, 0xffffffff, 0},
        {"ReverseGravityRoomPressGroundC", "SE_OJ_METAL_FLOOR_S_START", "SE_OJ_LV_METAL_FLOOR_S_MOVE", "SE_OJ_METAL_FLOOR_S_STOP", 0xffffffff, 0},
        {"ReverseGravityRoomPressGroundD", "SE_OJ_METAL_FLOOR_L_START", "SE_OJ_LV_METAL_FLOOR_L_MOVE", "SE_OJ_METAL_FLOOR_L_STOP", 0xffffffff, 0},
        {"BattleShipMovePartsB", "SE_OJ_BS_MOVE_PARTS_B_ST", "SE_OJ_LV_BS_MOVE_PARTS_B_MV", "SE_OJ_BS_MOVE_PARTS_B_ED", 0xffffffff, 0},
        {"IceWaterUpDownParts", 0, "SE_AT_LV_ICE_WATER_UP_DOWN", 0, 0xffffffff, 0},
        {"LavaUpDownParts", 0, "SE_AT_LV_LAVA_UP_DOWN", 0, 0xffffffff, 0},
        {"KoopaVS2PartsStartMoveStepA", "SE_OJ_KP2_START_STEP_ST", "SE_OJ_LV_KP2_START_STEP_MV", "SE_OJ_KP2_START_STEP_ED", 0xffffffff, 0},
        {"KoopaVS2PartsStartMoveStepB", "SE_OJ_KP2_START_STEP_ST", "SE_OJ_LV_KP2_START_STEP_MV", "SE_OJ_KP2_START_STEP_ED", 0xffffffff, 0},
        {"KoopaVS2PartsStartMoveStepC", "SE_OJ_KP2_START_STEP_ST", "SE_OJ_LV_KP2_START_STEP_MV", "SE_OJ_KP2_START_STEP_ED", 0xffffffff, 0},
        {"KoopaVS2PartsRollingStep", 0, "SE_OJ_LV_KP2_ROLL_STEP_MV", 0, 0xffffffff, 0},
        {"KoopaVS2PartsSSquareMoveStep", "SE_OJ_KP2_S_SQR_STEP_ST", "SE_OJ_LV_KP2_S_SQR_STEP_MV", "SE_OJ_KP2_S_SQR_STEP_ED", 0xffffffff, 0},
        {"KoopaVS2Parts2DMoveStepBarA", "SE_OJ_KP2_START_STEP_ST_F", "SE_OJ_LV_KP2_START_STEP_M_F", "SE_OJ_KP2_START_STEP_ED_F", 0xffffffff, 0},
        {"KoopaVS2Parts2DMoveStepBarB", "SE_OJ_KP2_START_STEP_ST_F", "SE_OJ_LV_KP2_START_STEP_M_F", "SE_OJ_KP2_START_STEP_ED_F", 0xffffffff, 0},
        {"KoopaVS2Parts2DMoveStepSBarB", "SE_OJ_KP2_START_STEP_ST_F", "SE_OJ_LV_KP2_START_STEP_M_F", "SE_OJ_KP2_START_STEP_ED_F", 0xffffffff, 0},
        {"KoopaVS2Parts2DMoveStepConvex", "SE_OJ_KP2_START_STEP_ST_F", "SE_OJ_LV_KP2_START_STEP_M_F", "SE_OJ_KP2_START_STEP_ED_F", 0xffffffff, 0},
        {"KoopaVS2Parts2DMoveStepLShape", "SE_OJ_KP2_START_STEP_ST_F", "SE_OJ_LV_KP2_START_STEP_M_F", "SE_OJ_KP2_START_STEP_ED_F", 0xffffffff, 0},
        {"KoopaVS2Parts2DMoveStepCross", "SE_OJ_KP2_START_STEP_ST_F", "SE_OJ_LV_KP2_START_STEP_M_F", "SE_OJ_KP2_START_STEP_ED_F", 0xffffffff, 0},
        {"KoopaVS2PartsSquareMoveStepA", 0, "SE_OJ_LV_KP2_START_STEP_MV", 0, 0xffffffff, 0},
        {"KoopaVS2PartsSquareMoveStepB", 0, "SE_OJ_LV_KP2_START_STEP_MV", 0, 0xffffffff, 0},
        {"KoopaVS2PartsJoinedMoveStep", "SE_OJ_KP2_JNT_STEP_ST", "SE_OJ_LV_KP2_JNT_STEP_MV", "SE_OJ_KP2_JNT_STEP_ED", 0xffffffff, 0},
        {"HeavenlyBeachTimerStepA", "SE_OJ_O_APPEAR_BRDG_APPEAR", 0, 0, 0xffffffff, 0},
        {"HeavenlyBeachTimerStepB", "SE_OJ_O_APPEAR_BRDG_APPEAR", 0, 0, 0xffffffff, 0},
        {"IceLavaIslandLavaMovableStepA", 0, "SE_AT_LV_ICE_LAVA_LAVA_STEP", 0, 0xffffffff, 0},
        {"ConcreteBlock", 0, 0, "SE_OJ_STRANGEROCK_BREAK_M", 0xffffffff, 0},
        {"WindMillPropeller", 0, "SE_OJ_LV_WINDMILL_PROPELLER", 0, 0xffffffff, 0},
        {"BiriBiriBegomanSpikePistonA", "SE_OJ_METAL_FLOOR_L_START", "SE_OJ_LV_METAL_FLOOR_L_MOVE", "SE_OJ_METAL_FLOOR_L_STOP", 0xffffffff, 0},
        {"HeavenlyBeachUnderRock", "SE_OJ_HB_UNDERROCK_BREAK", "SE_OJ_LV_HB_UNDERROCK_WATER", 0, 0xffffffff, 0},
        {"HeavenlyBeachTopRock", "SE_OJ_STRANGEROCK_BREAK_M", 0, 0, 0xffffffff, 0},
        {"FlagDiscPlanetD", 0, "SE_OJ_LV_FLY_DISK_ROTATE", 0, 0xffffffff, 0},
        {"IceVolcanoAppearStepB", "SE_OJ_IV_STEP_B_START", "SE_OJ_LV_IV_STEP_B_MVOE", "SE_OJ_IV_STEP_B_END", 0xd2, 0},
        {"ShutterDoorE", "SE_OJ_MTL_SHUTTER_S_ST", "SE_OJ_LV_MTL_SHUTTER_S_MV", "SE_OJ_MTL_SHUTTER_S_ED", 0xad, 2},
        {"SnowBlockA", 0, 0, "SE_OJ_SNOW_MAN_MELT", 0xffffffff, 0},
        {"ChallengeBallRotateBridgeA", "SE_OJ_CHAL_RT_BRIDGE_ST", "SE_OJ_LV_CHAL_RT_BRIDGE_ROT", "SE_OJ_CHAL_RT_BRIDGE_ED", 0xffffffff, 0},
        {"ChallengeBallMoveGroundA", "SE_OJ_CHAL_MV_GND_L_ST", "SE_OJ_LV_CHAL_MV_GND_L_MV", "SE_OJ_CHAL_MV_GND_L_ED", 0xffffffff, 0},
        {"ChallengeBallMoveGroundB", 0, "SE_OJ_LV_CHAL_MV_GND_S_MV", 0, 0xffffffff, 0},
        {"ChallengeBallGearA", 0, "SE_OJ_LV_CHAL_GEAR_ROTATE", 0, 0xffffffff, 0},
        {"ChallengeBallAccelCylinderA", 0, "SE_OJ_LV_CHAL_ACC_CYLINDER", 0, 0xffffffff, 0},
        {"AstroDecoratePartsGearA", 0, "SE_OJ_LV_ASTRO_DECO_GEAR_MV", 0, 0xffffffff, 0},
        {"DarkHopperRotateStepA", "SE_OJ_MYST_CIRCLE_PENDULUM", 0, 0, 0xffffffff, 0},
        {"TrialBubbleRevolvingPartsA", 0, "SE_OJ_LV_TRIBBL_REVOLV_PRT", 0, 0xffffffff, 0},
        {"TrialBubbleRotateWallA", 0, "SE_OJ_LV_TRIBBL_ROT_WALL", 0, 0xffffffff, 0},
        {"TrialBubbleMoveWallA", "SE_OJ_TRIBBL_MOVEWALL_ST", "SE_OJ_LV_TRIBBL_MOVEWALL_MV", "SE_OJ_TRIBBL_MOVEWALL_ED", 0xffffffff, 0},
        {"CubeBubbleExRotateWallL", 0, "SE_OJ_LV_CUBEBBL_ROT_WALL_L", 0, 0xffffffff, 0},
        {"PowerStarKeeperA", 0, "SE_OJ_LV_POW_STAR_KEEPER_A", 0, 0xffffffff, 0},
        {"PowerStarKeeperB", 0, "SE_OJ_LV_POW_STAR_KEEPER_B", 0, 0xffffffff, 0},
        {"PowerStarKeeperC", 0, "SE_OJ_LV_POW_STAR_KEEPER_C", 0, 0xffffffff, 0},
        {"KoopaVS3RotateStepsA", 0, "SE_OJ_LV_KP_VS3_ROT_STEP_A", 0, 0xffffffff, 0},
        {"DeathPromenadeMovePartsSpuareA", "SE_OJ_DP_MOVE_STEP_START", "SE_OJ_LV_DP_MOVE_STEP_MOVE", "SE_OJ_DP_MOVE_STEP_START", 0xffffffff, 0},
        {"DeathPromenadeRotateCircleL", 0, "SE_OJ_LV_DP_ROT_CIRCLE_L", 0, 0xffffffff, 0},
        {"DeathPromenadeRotateCircleS", 0, "SE_OJ_LV_DP_ROT_CIRCLE_S", 0, 0xffffffff, 0},
        {"MiniMechaKoopaPartsFan", 0, "SE_OJ_LV_MM_KOOPA_FAN", 0, 0xffffffff, 0},
        {"ClockworkHandle", 0, "SE_OJ_LV_CLOCKWORK_HANDLE", 0, 0xffffffff, 0},
        {"SunkenShip", 0, 0, "SE_OJ_SUNKEN_SHIP_BREAK", 0xffffffff, 2}};

    StageEffectCameraData* getStageEffectCameraParam(const char* pName) {
        for (u32 i = 0; i < sizeof(sStageEffectCameraDataTable) / sizeof(StageEffectCameraData); i++) {
            if (MR::isEqualString(pName, sStageEffectCameraDataTable[i].objectName)) {
                return &sStageEffectCameraDataTable[i];
            }
        }
        return nullptr;
    }

    StageEffectPadData* getStageEffectPadParam(const char* pName) {
        for (u32 i = 0; i < sizeof(sStageEffectPadDataTable) / sizeof(StageEffectPadData); i++) {
            if (MR::isEqualString(pName, sStageEffectPadDataTable[i].objectName)) {
                return &sStageEffectPadDataTable[i];
            }
        }
        return nullptr;
    }

    StageEffectSoundData* getStageEffectSeParam(const char* pName) {
        for (u32 i = 0; i < sizeof(sStageEffectSeDataTable) / sizeof(StageEffectSoundData); i++) {
            if (MR::isEqualString(pName, sStageEffectSeDataTable[i].objectName)) {
                return &sStageEffectSeDataTable[i];
            }
        }
        return nullptr;
    }

    void shakeCamera(CameraShakeType type) {
        switch (type) {
        case Strong:
            MR::shakeCameraStrong();
            break;
        case Normal:
            MR::shakeCameraNormal();
            break;
        case NormalWeak:
            MR::shakeCameraNormalWeak();
            break;
        case Weak:
            MR::shakeCameraWeak();
            break;
        case VeryWeak:
            MR::shakeCameraVeryWeak();
            break;
        default:
            break;
        }
    }

    bool isDistanceValidShake(LiveActor* pActor, const char* pName) {
        f32 range = getStageEffectCameraParam(pName) != nullptr ? getStageEffectCameraParam(pName)->distance : 0.0f;
        if (range == -1.0f) {
            return MR::isOnPlayer(pActor);
        }

        f32 distance = MR::calcDistanceToPlayer(pActor);
        return 0.0f < distance && distance < (getStageEffectCameraParam(pName) != nullptr ? getStageEffectCameraParam(pName)->distance : 0.0f);
    }
}

namespace MR {
    const char* StageEffect::getStartSe(const char* pName) {
        return getStageEffectSeParam(pName) != nullptr ? getStageEffectSeParam(pName)->startSoundEffect : nullptr;
    }

    const char* StageEffect::getMovingSe(const char* pName) {
        return getStageEffectSeParam(pName) != nullptr ? getStageEffectSeParam(pName)->movingSoundEffect : nullptr;
    }

    const char* StageEffect::getStopSe(const char* pName) {
        return getStageEffectSeParam(pName) != nullptr ? getStageEffectSeParam(pName)->stopSoundEffect : nullptr;
    }

    s32 StageEffect::getStopSeSteps(const char* pName) {
        return getStageEffectSeParam(pName) != nullptr ? getStageEffectSeParam(pName)->stopSoundEffectSteps : -1;
    }

    bool StageEffect::isRiddleSeTypeStop(const char* pName) {
        return getStageEffectSeParam(pName) != nullptr ? getStageEffectSeParam(pName)->soundEffectType == 2 : false;
    }

    bool StageEffect::isExistStageEffectSeData(const char* pName) {
        return getStageEffectSeParam(pName) != nullptr;
    }

    bool StageEffect::isExistStageEffectData(const char* pName) {
        return getStageEffectCameraParam(pName) != nullptr || getStageEffectPadParam(pName) != nullptr || getStageEffectSeParam(pName) != nullptr;
    }

    void StageEffect::shakeStartCamera(LiveActor* pActor, const char* pName) {
        if (isDistanceValidShake(pActor, pName)) {
            shakeCamera(getStageEffectCameraParam(pName) != nullptr ? getStageEffectCameraParam(pName)->startShake : None);
        }
    }

    void StageEffect::shakeStopCamera(LiveActor* pActor, const char* pName) {
        if (isDistanceValidShake(pActor, pName)) {
            shakeCamera(getStageEffectCameraParam(pName) != nullptr ? getStageEffectCameraParam(pName)->stopShake : None);
        }
    }

    void StageEffect::shakeCameraMoving(NameObj* pActor, const char* pName) {
        f32 intensity = getStageEffectCameraParam(pName) != nullptr ? getStageEffectCameraParam(pName)->movingIntensity : 0.0f;
        if (intensity <= 0.0f) {
            return;
        }

        f32 speed = getStageEffectCameraParam(pName) != nullptr ? getStageEffectCameraParam(pName)->movingSpeed : 0.0f;
        MR::shakeCameraInfinity(pActor, intensity, speed);
    }

    void StageEffect::stopShakingCameraMoving(NameObj* pActor, const char* pName) {
        f32 intensity = getStageEffectCameraParam(pName) != nullptr ? getStageEffectCameraParam(pName)->movingIntensity : 0.0f;
        if (intensity <= 0.0f) {
            return;
        }

        MR::stopShakingCamera(pActor);
    }

    void StageEffect::rumblePadStart(LiveActor* pActor, const char* pName) {
        if (!isDistanceValidShake(pActor, pName)) {
            return;
        }

        const char* pPattern = getStageEffectPadParam(pName) != nullptr ? getStageEffectPadParam(pName)->startPattern : nullptr;
        if (pPattern != nullptr) {
            MR::tryRumblePad(pActor, pPattern, 0);
        }
    }

    void StageEffect::rumblePadStop(LiveActor* pActor, const char* pName) {
        if (!isDistanceValidShake(pActor, pName)) {
            return;
        }

        const char* pPattern = getStageEffectPadParam(pName) != nullptr ? getStageEffectPadParam(pName)->stopPattern : nullptr;
        if (pPattern != nullptr) {
            MR::tryRumblePad(pActor, pPattern, 0);
        }
    }

    void StageEffect::rumblePadMoving(LiveActor* pActor, const char* pName) {
        const char* pPattern = getStageEffectPadParam(pName) != nullptr ? getStageEffectPadParam(pName)->movingPattern : nullptr;
        if (pPattern != nullptr) {
            MR::tryRumblePad(pActor, pPattern, 0);
        }
    }

    bool StageEffect::tryStageEffectStart(LiveActor* pActor, const char* pName) {
        if (!isExistStageEffectData(pName)) {
            return false;
        }

        if (MR::isRegisteredEffect(pActor, "Start")) {
            MR::emitEffect(pActor, "Start");
        }

        const char* pSound = getStartSe(pName);
        if (pSound != nullptr) {
            MR::startSound(pActor, pSound, -1, -1);
        }

        bool isRiddle = getStageEffectSeParam(pName) != nullptr ? getStageEffectSeParam(pName)->soundEffectType == 1 : false;
        if (isRiddle) {
            MR::startSystemSE("SE_SY_READ_RIDDLE_S", -1, -1);
        }

        rumblePadStart(pActor, pName);
        shakeStartCamera(pActor, pName);
        return true;
    }

    bool StageEffect::tryStageEffectMoving(LiveActor* pActor, const char* pName) {
        if (!isExistStageEffectData(pName)) {
            return false;
        }

        const char* pSound = getMovingSe(pName);
        if (pSound != nullptr) {
            MR::startLevelSound(pActor, pSound, -1, -1, -1);
        }

        rumblePadMoving(pActor, pName);
        return true;
    }

    bool StageEffect::tryStageEffectStop(LiveActor* pActor, const char* pName) {
        if (!isExistStageEffectData(pName)) {
            return false;
        }

        if (MR::isRegisteredEffect(pActor, "Stop")) {
            MR::emitEffect(pActor, "Stop");
        }

        const char* pSound = getStopSe(pName);
        if (pSound != nullptr) {
            MR::startSound(pActor, pSound, -1, -1);
        }

        if (isRiddleSeTypeStop(pName)) {
            MR::startSystemSE("SE_SY_READ_RIDDLE_S", -1, -1);
        }

        rumblePadStop(pActor, pName);
        shakeStopCamera(pActor, pName);
        return true;
    }
}
