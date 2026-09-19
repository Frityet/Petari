#include "compat/Cp932Literal.hpp"
#pragma once

#include "Game/Animation/XanimeResource.hpp"

XanimeBckTable4 quadAnimeTable[] = {
    {
        {CP932("基本")},        // mParent
        "WalkSoft",      // fileName1
        0.00000000000f,  // 0x8
        "Walk",          // fileName2
        0.00000000000f,  // 0x10
        "Run",           // fileName3
        0.00000000000f,  // 0x18
        "Wait",          // fileName4
        1.00000000000f,  // 0x20
    },
    {
        {CP932("水泳基本")},          // mParent
        "SwimWait",            // fileName1
        1.00000000000f,        // 0x8
        "SwimFlutterSurface",  // fileName2
        0.00000000000f,        // 0x10
        "SwimFlutter",         // fileName3
        0.00000000000f,        // 0x18
        "SwimDrift",           // fileName4
        0.00000000000f,        // 0x20
    },
    {
        {CP932("テニス基本")},  // mParent
        "WalkSoft",      // fileName1
        0.00000000000f,  // 0x8
        "Walk",          // fileName2
        0.00000000000f,  // 0x10
        "TennisRun",     // fileName3
        0.00000000000f,  // 0x18
        "TennisWait",    // fileName4
        1.00000000000f,  // 0x20
    },
    {
        {CP932("スライダー尻")},    // mParent
        "SlideHipForWard",   // fileName1
        0.300000011921f,     // 0x8
        "SlideHipBackward",  // fileName2
        0.200000002980f,     // 0x10
        "SlideHipLeft",      // fileName3
        0.250000000000f,     // 0x18
        "SlideHipRight",     // fileName4
        0.250000000000f,     // 0x20
    },
    {
        {CP932("坂すべり下向きあおむけ")},  // mParent
        "SlideHipForWard",           // fileName1
        0.300000011921f,             // 0x8
        "SlideHipBackward",          // fileName2
        0.200000002980f,             // 0x10
        "SlideHipLeft",              // fileName3
        0.250000000000f,             // 0x18
        "SlideHipRight",             // fileName4
        0.250000000000f,             // 0x20
    },
    {
        {CP932("タマコロ移動")},  // mParent
        "BallIdle",        // fileName1
        1.00000000000f,    // 0x8
        "BallWalkSoft",    // fileName2
        0.00000000000f,    // 0x10
        "BallWalk",        // fileName3
        0.00000000000f,    // 0x18
        "BallRun",         // fileName4
        0.00000000000f,    // 0x20
    },
    {
        {""},            // mParent
        "",              // fileName1
        0.00000000000f,  // 0x8
        nullptr,         // fileName2
        0.00000000000f,  // 0x10
        nullptr,         // fileName3
        0.00000000000f,  // 0x18
        nullptr,         // fileName4
        0.00000000000f,  // 0x20
    },
};

XanimeBckTable3 tripleAnimeTable[] = {
    {
        {CP932("坂左右ウエイト")},  // mParent
        "Wait",              // fileName1
        1.00000000000f,      // 0x8
        "WaitSlopeL",        // fileName2
        0.00000000000f,      // 0x10
        "WaitSlopeR",        // fileName3
        0.00000000000f,      // 0x18
    },
    {
        {CP932("坂前後ウエイト")},  // mParent
        "Wait",              // fileName1
        1.00000000000f,      // 0x8
        "WaitSlopeD",        // fileName2
        0.00000000000f,      // 0x10
        "WaitSlopeU",        // fileName3
        0.00000000000f,      // 0x18
    },
    {
        {CP932("サーフィン")},   // mParent
        "SurfRideLoop",   // fileName1
        1.00000000000f,   // 0x8
        "SurfRideLoopL",  // fileName2
        0.00000000000f,   // 0x10
        "SurfRideLoopR",  // fileName3
        0.00000000000f,   // 0x18
    },
    {
        {CP932("サーフィン（加速）")},  // mParent
        "SurfRideDashLoop",      // fileName1
        1.00000000000f,          // 0x8
        "SurfRideDashLoopL",     // fileName2
        0.00000000000f,          // 0x10
        "SurfRideDashLoopR",     // fileName3
        0.00000000000f,          // 0x18
    },
    {
        {CP932("サーフィン傾き開始")},  // mParent
        "SurfRide",              // fileName1
        1.00000000000f,          // 0x8
        "SurfRideL",             // fileName2
        0.00000000000f,          // 0x10
        "SurfRideR",             // fileName3
        0.00000000000f,          // 0x18
    },
    {
        {CP932("サーフィン傾き開始（加速）")},  // mParent
        "SurfRideDash",                  // fileName1
        1.00000000000f,                  // 0x8
        "SurfRideDashL",                 // fileName2
        0.00000000000f,                  // 0x10
        "SurfRideDashR",                 // fileName3
        0.00000000000f,                  // 0x18
    },
    {
        {CP932("サーフィン落下")},  // mParent
        "SurfFall",          // fileName1
        1.00000000000f,      // 0x8
        "SurfFallL",         // fileName2
        0.00000000000f,      // 0x10
        "SurfFallR",         // fileName3
        0.00000000000f,      // 0x18
    },
    {
        {""},            // mParent
        "",              // fileName1
        0.00000000000f,  // 0x8
        nullptr,         // fileName2
        0.00000000000f,  // 0x10
        nullptr,         // fileName3
        0.00000000000f,  // 0x18
    },
};

XanimeBckTable2 doubleAnimeTable[] = {
    {
        {CP932("しゃがみ基本")},  // mParent
        "SquatWait",       // fileName1
        1.00000000000f,    // 0x8
        "SquatWalk",       // fileName2
        0.00000000000f,    // 0x10
    },
    {
        {CP932("その場足踏み")},  // mParent
        "Run",             // fileName1
        0.750000000000f,   // 0x8
        "Wait",            // fileName2
        0.250000000000f,   // 0x10
    },
    {
        {CP932("タマコロしゃがみ")},  // mParent
        "BallSquat",           // fileName1
        1.00000000000f,        // 0x8
        "BallWalk",            // fileName2
        0.00000000000f,        // 0x10
    },
    {
        {""},            // mParent
        "",              // fileName1
        0.00000000000f,  // 0x8
        nullptr,         // fileName2
        0.00000000000f,  // 0x10
    },
};

XanimeBckTable1 singleAnimeTable[] = {
    {
        {CP932("ジャンプ")},  // mParent
        "Jump",        // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("ジャンプB")},  // mParent
        "Jump2",        // fileName
        0,              // animationHash
        0,              // fileHash
    },
    {
        {CP932("ジャンプC")},  // mParent
        "JumpRoll",     // fileName
        0,              // animationHash
        0,              // fileHash
    },
    {
        {CP932("ターンジャンプ")},  // mParent
        "JumpTurn",          // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("空中一回転")},  // mParent
        "AirControl",    // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("フリージャンプ")},  // mParent
        "Rise",              // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ヘリコプタージャンプ")},  // mParent
        "FlickAir",                // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("ダッシュジャンプ")},  // mParent
        "Rolling",             // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("飛びすさりジャンプ")},  // mParent
        "Bounce",                // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("ショートジャンプ")},  // mParent
        "GravityChange",       // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("スカイラブジャンプ")},  // mParent
        "JumpTwin",              // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("スケキヨ")},  // mParent
        "Bury",        // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("スケキヨ脱出")},  // mParent
        "Bury",            // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("埋まり")},       // mParent
        "BuryStandWait",  // fileName
        0,                // animationHash
        0,                // fileHash
    },
    {
        {CP932("埋まり脱出")},  // mParent
        "BuryStandEnd",  // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("逆着地")},       // mParent
        "CannonFlyLand",  // fileName
        0,                // animationHash
        0,                // fileHash
    },
    {
        {CP932("ホッパージャンプA")},  // mParent
        "HopperWaitA",          // fileName
        0,                      // animationHash
        0,                      // fileHash
    },
    {
        {CP932("ホッパージャンプB")},  // mParent
        "HopperWaitB",          // fileName
        0,                      // animationHash
        0,                      // fileHash
    },
    {
        {CP932("ホッパーふみジャンプA")},  // mParent
        "HopperWaitA",              // fileName
        0,                          // animationHash
        0,                          // fileHash
    },
    {
        {CP932("ホッパーふみジャンプB")},  // mParent
        "HopperWaitB",              // fileName
        0,                          // animationHash
        0,                          // fileHash
    },
    {
        {CP932("ホッパー移動A")},  // mParent
        "HopperRunA",       // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("ホッパー移動B")},  // mParent
        "HopperRunB",       // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("ホッパーハイジャンプA")},  // mParent
        "HopperJumpA",              // fileName
        0,                          // animationHash
        0,                          // fileHash
    },
    {
        {CP932("ホッパーハイジャンプB")},  // mParent
        "HopperJumpB",              // fileName
        0,                          // animationHash
        0,                          // fileHash
    },
    {
        {CP932("ホッパー壁ジャンプ")},  // mParent
        "HopperWallJump",        // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("ホッパーヒップドロップ開始")},  // mParent
        "HopperMarioHipDropStart",       // fileName
        0,                               // animationHash
        0,                               // fileHash
    },
    {
        {CP932("ホッパーヒップドロップ")},  // mParent
        "HopperMarioHipDrop",        // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("ジャンプふみ1")},  // mParent
        "Jump",             // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("ジャンプふみ2")},  // mParent
        "JumpPress2nd",     // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("ジャンプふみ3")},  // mParent
        "JumpPress3rd",     // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("引き戻し")},  // mParent
        "PullBack",    // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("引き戻し着地")},  // mParent
        "PullBackLand",    // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ポッドワープ開始")},  // mParent
        "WarpPodStart",        // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("ポッドワープ終了")},  // mParent
        "WarpPodEnd",          // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("幅とび")},   // mParent
        "JumpBroad",  // fileName
        0,            // animationHash
        0,            // fileHash
    },
    {
        {CP932("着地")},  // mParent
        "Land",    // fileName
        0,         // animationHash
        0,         // fileHash
    },
    {
        {CP932("着地B")},    // mParent
        "Jump2Land",  // fileName
        0,            // animationHash
        0,            // fileHash
    },
    {
        {CP932("着地C")},       // mParent
        "JumpRollLand",  // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("着地ターン")},  // mParent
        "JumpTurnLand",  // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("着地幅とび")},   // mParent
        "JumpBroadLand",  // fileName
        0,                // animationHash
        0,                // fileHash
    },
    {
        {CP932("ハード着地")},  // mParent
        "LandStiffen",   // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("すべり着地")},  // mParent
        "LandSlope",     // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("ショート着地")},  // mParent
        "Land",            // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ヒップドロップ開始")},  // mParent
        "HipDropStart",          // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("ヒップドロップ")},  // mParent
        "HipDrop",           // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ヒップドロップ着地")},  // mParent
        "HipDropLand",           // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("ヒップドロップ滑り")},  // mParent
        "LandRotation",          // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("ジャンプ順滑り")},  // mParent
        "LandRotation",      // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ジャンプ逆滑り")},  // mParent
        "Fall",              // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("スピンヒップドロップ開始")},  // mParent
        "HipDropHomingStart",          // fileName
        0,                             // animationHash
        0,                             // fileHash
    },
    {
        {CP932("スピンヒップドロップ")},  // mParent
        "HipDropHoming",           // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("スピンヒップドロップ着地")},  // mParent
        "HipDropHomingLand",           // fileName
        0,                             // animationHash
        0,                             // fileHash
    },
    {
        {CP932("スリップアップ")},  // mParent
        "SlipUp",            // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("つかまりスリップアップ準備")},  // mParent
        "HangSlipUpStart",               // fileName
        0,                               // animationHash
        0,                               // fileHash
    },
    {
        {CP932("つかまりスリップアップ")},  // mParent
        "HangSlipUp",                // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("ハチ飛行中")},  // mParent
        "BeeFly",        // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("ハチ飛行中無入力")},  // mParent
        "BeeFlyWait",          // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("ハチ壁くっつき")},  // mParent
        "BeeLand",           // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ハチ壁くっつき中")},  // mParent
        "BeeWait",             // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("ハチ花移動")},      // mParent
        "BeeCreepWallWalk",  // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ハチジャンプ")},  // mParent
        "BeeJump",         // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ハチ壁ジャンプ")},  // mParent
        "BeeWallJump",       // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ハチ匍匐ウエイト")},  // mParent
        "BeeCreepWait",        // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("ハチ匍匐前進")},  // mParent
        "BeeCreepWalk",    // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ハチ匍匐着地")},  // mParent
        "BeeCreepLand",    // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ハチヒップドロップ開始")},  // mParent
        "BeeHipDropStart",           // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("ハチヒップドロップ")},  // mParent
        "BeeHipDrop",            // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("ハチヒップドロップ着地")},  // mParent
        "BeeHipDropLand",            // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("ハチヒップドロップ壁着地")},  // mParent
        "BeeHipDropLand",              // fileName
        0,                             // animationHash
        0,                             // fileHash
    },
    {
        {CP932("泥低速歩行")},  // mParent
        "WalkSoft",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("泥高速歩行")},  // mParent
        "WalkBury",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("落下")},  // mParent
        "Fall",    // fileName
        0,         // animationHash
        0,         // fileHash
    },
    {
        {CP932("フーファイター飛行開始")},  // mParent
        "FooStart",                  // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("フーファイター飛行")},  // mParent
        "FooFly",                // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("フーファイター飛行再開")},  // mParent
        "FooFlyStart",               // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("フーファイター静止")},  // mParent
        "FooWait",               // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("フーファイター解除")},  // mParent
        "FooEnd",                // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("フーファイター着地")},  // mParent
        "LandSlope",             // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("フーファイタースピン")},  // mParent
        "FooSpin",                 // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("壁ジャンプ")},  // mParent
        "WallJump",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("壁すべり")},  // mParent
        "WallSlide",   // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("壁くっつき")},  // mParent
        "WallKeep",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("壁押し")},  // mParent
        "Push",      // fileName
        0,           // animationHash
        0,           // fileHash
    },
    {
        {CP932("壁左歩き")},  // mParent
        "WallWalkL",   // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("壁右歩き")},  // mParent
        "WallWalkR",   // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("壁ウエイト")},  // mParent
        "WallWait",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("前壁ウエイト")},  // mParent
        "Push",            // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("壁ヒット")},  // mParent
        "WallHit",     // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("壁ヒット着地")},  // mParent
        "WallHitLand",     // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("壁はじき")},  // mParent
        "WallHit",     // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("崖ふんばり")},  // mParent
        "Stagger",       // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("カリカリ限界")},  // mParent
        "WaitHold",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("坂左ウエイト")},  // mParent
        "WaitSlopeL",      // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("坂右ウエイト")},  // mParent
        "WaitSlopeR",      // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("坂前ウエイト")},  // mParent
        "WaitSlopeD",      // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("坂後ウエイト")},  // mParent
        "WaitSlopeU",      // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ラン")},  // mParent
        "Run",     // fileName
        0,         // animationHash
        0,         // fileHash
    },
    {
        {CP932("歩行")},  // mParent
        "Walk",    // fileName
        0,         // animationHash
        0,         // fileHash
    },
    {
        {CP932("鈍行")},    // mParent
        "WalkSoft",  // fileName
        0,           // animationHash
        0,           // fileHash
    },
    {
        {CP932("メタルダッシュ")},  // mParent
        "RunDash",           // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("埋まり歩行")},  // mParent
        "WalkBury",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("埋まりジャンプA")},  // mParent
        "WalkBuryJumpLow",    // fileName
        0,                    // animationHash
        0,                    // fileHash
    },
    {
        {CP932("埋まりジャンプB")},  // mParent
        "WalkBuryJumpLow2",   // fileName
        0,                    // animationHash
        0,                    // fileHash
    },
    {
        {CP932("埋まり脱出ジャンプ")},  // mParent
        "WalkBuryJumpHi",        // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("つぶれ")},  // mParent
        "Press",     // fileName
        0,           // animationHash
        0,           // fileHash
    },
    {
        {CP932("つぶれ復帰")},  // mParent
        "PressRecover",  // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("つぶれ解除")},  // mParent
        "Jump",          // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("ブレーキ")},  // mParent
        "Brake",       // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("ブレーキ滑り床")},  // mParent
        "Run",               // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ターンブレーキ")},  // mParent
        "Turn",              // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ターンブレーキ滑り床")},  // mParent
        "Run",                     // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("歩行制動ブレーキ")},  // mParent
        "RunEnd",              // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("氷上慣性走行")},  // mParent
        "Brake",           // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("氷上力行左")},  // mParent
        "SkateL",        // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("氷上力行右")},  // mParent
        "SkateR",        // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("氷上切替左")},  // mParent
        "SkateSwitchL",  // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("氷上切替右")},  // mParent
        "SkateSwitchR",  // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("氷上後行左")},  // mParent
        "SkateBackL",    // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("氷上後行右")},  // mParent
        "SkateBackR",    // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("氷上ターン")},  // mParent
        "SkateTurn",     // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("しゃがみ")},  // mParent
        "SquatWait",   // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("しゃがみ開始")},  // mParent
        "SquatStart",      // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("しゃがみ終了")},  // mParent
        "SquatEnd",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("坂すべり上向きうつぶせ")},  // mParent
        "SlideStmach",               // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("坂すべり上向き終了")},  // mParent
        "SlideStmachEnd",        // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("坂すべり下向き終了")},  // mParent
        "SlideHipEnd",           // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("特殊ウエイト1A")},  // mParent
        "Sleep",             // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("特殊ウエイト1B")},  // mParent
        "SleepLie",          // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("戦闘ウエイト")},  // mParent
        "BattleWait",      // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("空転")},  // mParent
        "Run",     // fileName
        0,         // animationHash
        0,         // fileHash
    },
    {
        {CP932("ひろい")},    // mParent
        "CarryStart",  // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("ひろい空中")},   // mParent
        "CarryAirStart",  // fileName
        0,                // animationHash
        0,                // fileHash
    },
    {
        {CP932("ひろいクイック")},  // mParent
        "CarryStartShort",   // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ひろいウエイト")},  // mParent
        "CarryWait",         // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("カブ抜き")},  // mParent
        "PullOut",     // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("カブウエイト")},  // mParent
        "PullOutWait",     // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ハンマー投げ回転中")},  // mParent
        "Swing",                 // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("ハンマー投げリリース")},  // mParent
        "SwingThrow",              // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("コインゲット")},  // mParent
        "CoinGet",         // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("パンチ")},  // mParent
        "Spin2nd",   // fileName
        0,           // animationHash
        0,           // fileHash
    },
    {
        {CP932("空パンチ")},  // mParent
        "Spin2nd",     // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("左パンチ")},  // mParent
        "Spin2nd",     // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("左空パンチ")},  // mParent
        "Spin2nd",       // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("ランニングキック")},  // mParent
        "Kick",                // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("アッパーパンチ")},  // mParent
        "Jump",              // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("しゃがみアッパー")},  // mParent
        "SquatEnd",            // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("サマーソルト")},  // mParent
        "SpinLow",         // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("投げ")},  // mParent
        "Throw",   // fileName
        0,         // animationHash
        0,         // fileHash
    },
    {
        {CP932("ファイア投げ")},  // mParent
        "Throw",           // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("両手投げ")},  // mParent
        "ThrowBoth",   // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("ファイアスピン")},  // mParent
        "FireSpin",          // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ファイアスピン空中")},  // mParent
        "FireSpin",              // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("崖つかまり開始")},  // mParent
        "HangStartUnder",    // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("崖つかまり中")},  // mParent
        "HangWait",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("崖つかまり終了")},  // mParent
        "HangUp",            // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("崖つかまり終了坂")},  // mParent
        "HangUp",              // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("崖つかまり降り")},  // mParent
        "HangStart",         // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("しゃがみジャンプ")},  // mParent
        "JumpBack",            // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("しゃがみジャンプ着地")},  // mParent
        "JumpBackLand",            // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("しゃがみ歩き")},  // mParent
        "SquatWalk",       // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("がんばり走り")},  // mParent
        "RunSlope",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("腹ばいジャンプ")},     // mParent
        "SlideStomachRecover",  // fileName
        0,                      // animationHash
        0,                      // fileHash
    },
    {
        {CP932("尻滑りジャンプ")},  // mParent
        "SlideHipRecover",   // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("向かい風走り")},  // mParent
        "RunSlope",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("向かい風ふんばり")},  // mParent
        "Stagger",             // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("その場足踏み上半身")},  // mParent
        "Walk",                  // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("水泳ウエイト")},  // mParent
        "SwimWait",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("水泳一掻き")},  // mParent
        "SwimBreast",    // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("水上一掻き")},       // mParent
        "SwimBreastSurface",  // fileName
        0,                    // animationHash
        0,                    // fileHash
    },
    {
        {CP932("水泳潜り")},  // mParent
        "SwimDive",    // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("水泳ジャンプダイブ")},  // mParent
        "LandWater",             // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("水泳ジャンプダイブ回転")},  // mParent
        "LandWaterDive",             // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("水泳上昇呼吸")},  // mParent
        "SwimRise",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("水泳水面初期移動")},  // mParent
        "SwimStartSurface",    // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("水上スピン")},     // mParent
        "SwimSpinSurface",  // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("水泳スピン")},  // mParent
        "SwimSpin",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("水泳スピン移動")},  // mParent
        "SwimSpinAttack",    // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("水上スピン移動")},       // mParent
        "SwimSpinAttackSurface",  // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("水泳スピンジャンプ")},  // mParent
        "SwimJump",              // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("水泳ダメージ")},   // mParent
        "SwimDamageSmall",  // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("水泳ダメージ亀")},             // mParent
        "SwimFlutterBoardDamageSmall",  // fileName
        0,                              // animationHash
        0,                              // fileHash
    },
    {
        {CP932("水泳ダメージ中")},  // mParent
        "SwimDamageMiddle",  // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("水上ダメージ中")},         // mParent
        "SwimDamageMiddleSurface",  // fileName
        0,                          // animationHash
        0,                          // fileHash
    },
    {
        {CP932("水上ダメージ着水")},           // mParent
        "SwimDamageMiddleSurfaceLand",  // fileName
        0,                              // animationHash
        0,                              // fileHash
    },
    {
        {CP932("水泳ジェット")},    // mParent
        "SwimFlutterboard",  // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("水泳ジェット終了")},  // mParent
        "SwimDamageMiddle",    // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("水泳亀投げ")},           // mParent
        "SwimFlutterboardThrow",  // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("水泳ジェット開始")},     // mParent
        "SwimFlutterboardStart",  // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("水泳ジェット壁ターン")},  // mParent
        "SwimFlutterboardTurn",    // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("水泳ダウン")},  // mParent
        "SwimDie",       // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("水泳ターン左")},  // mParent
        "SwimTurnL",       // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("水泳ターン右")},  // mParent
        "SwimTurnR",       // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("水泳壁ヒット")},  // mParent
        "SwimWallHit",     // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("水泳ターン下")},   // mParent
        "SwimTurnForward",  // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("水泳陸うちあげ")},  // mParent
        "SlideStmachEnd",    // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("飛び込み準備")},  // mParent
        "DiveWait",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("飛び込みジャンプ")},  // mParent
        "Dive",                // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("後方飛び込みジャンプ")},  // mParent
        "DiveBack",                // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("飛び込み失敗着地")},  // mParent
        "Land",                // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("飛び込み失敗回転着地")},  // mParent
        "LandRotation",            // fileName
        0,                         // animationHash
        0,                         // fileHash
    },
    {
        {CP932("リングダッシュ")},  // mParent
        "SwimDashRing",      // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("リングダッシュ準備")},  // mParent
        "SwimDashRingStart",     // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("カメ持ちリング準備")},           // mParent
        "SwimFlutterBoardDashRingStart",  // fileName
        0,                                // animationHash
        0,                                // fileHash
    },
    {
        {CP932("カメ持ちリング")},          // mParent
        "SwimFlutterBoardDashRing",  // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("匍匐前進")},  // mParent
        "SwimGetUp",   // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("前方小ダメージ")},  // mParent
        "DamageSmallFront",  // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("後方小ダメージ")},  // mParent
        "DamageSmallBack",   // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("強制ジャンプ")},  // mParent
        "Rise",            // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("中ダメージ")},       // mParent
        "DamageMiddleFront",  // fileName
        0,                    // animationHash
        0,                    // fileHash
    },
    {
        {CP932("中ダメージ空中")},      // mParent
        "DamageMiddleFrontAir",  // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("中ダメージ着地")},       // mParent
        "DamageMiddleFrontLand",  // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("中後ダメージ")},    // mParent
        "DamageMiddleBack",  // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("中後ダメージ空中")},   // mParent
        "DamageMiddleBackAir",  // fileName
        0,                      // animationHash
        0,                      // fileHash
    },
    {
        {CP932("中後ダメージ着地")},    // mParent
        "DamageMiddleBackLand",  // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("炎ダメージ")},  // mParent
        "DamageFire",    // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("ノーダメージ")},  // mParent
        "DamageBit",       // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("炎のランナー")},  // mParent
        "FireRun",         // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ファイアラン前兆")},  // mParent
        "FireRunStart",        // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("電気ダメージ")},  // mParent
        "DamageElectric",  // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("電気ダメージ終了")},  // mParent
        "DamageElectricEnd",   // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("ファイアダンス")},  // mParent
        "FireRun",           // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ダメージ")},   // mParent
        "DamageStart",  // fileName
        0,              // animationHash
        0,              // fileHash
    },
    {
        {CP932("ダメージ着地")},  // mParent
        "Land",            // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ダメージウエイト")},  // mParent
        "DamageWait",          // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("ノーマルウエイト")},  // mParent
        "Wait",                // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("氷結")},  // mParent
        "Freeze",  // fileName
        0,         // animationHash
        0,         // fileHash
    },
    {
        {CP932("氷結解除")},  // mParent
        "IceFlick",    // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("しびれ")},    // mParent
        "DamageNumb",  // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("しびれ回復")},   // mParent
        "DamageNumbEnd",  // fileName
        0,                // animationHash
        0,                // fileHash
    },
    {
        {CP932("はねとばされ")},  // mParent
        "DamageFlick",     // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("はねとばされ終了")},  // mParent
        "DamageFlickEnd",      // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("後転ふっとび")},   // mParent
        "DamageWeakFront",  // fileName
        0,                  // animationHash
        0,                  // fileHash
    },
    {
        {CP932("後転ふっとび空中")},  // mParent
        "DamageWeakFrontAir",  // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("後転ふっとび着地")},   // mParent
        "DamageWeakFrontLand",  // fileName
        0,                      // animationHash
        0,                      // fileHash
    },
    {
        {CP932("座りダウン")},  // mParent
        "DieSit",        // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("感電ダウン")},  // mParent
        "DieElectric",   // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("炎ダウン")},  // mParent
        "DieFire",     // fileName
        0,             // animationHash
        0,             // fileHash
    },
    {
        {CP932("仰向けダウン")},  // mParent
        "DieOver",         // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("俯せダウン")},  // mParent
        "DieUnder",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("奈落ダウン")},  // mParent
        "DieFall",       // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("ブラックホール落下")},  // mParent
        "DieBlackHole",          // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("つぶれダウン")},  // mParent
        "DieSit",          // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ゲームオーバー")},  // mParent
        "DieOver",           // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("レース負け")},  // mParent
        "DieEvent",      // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("水中レース負け")},  // mParent
        "DieSwimEvent",      // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("埋まりダウン")},  // mParent
        "DieBury",         // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("テニスショット左")},  // mParent
        "TennisShotL",         // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("テニスショット右")},  // mParent
        "TennisShotR",         // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("テニスショット中")},  // mParent
        "TennisShotM",         // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("テニスショット空")},  // mParent
        "TennisShotAir",       // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("テニスウエイト")},  // mParent
        "TennisWait",        // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("エレメントゲット")},  // mParent
        "ElementGet",          // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("エレメントゲット接地中")},  // mParent
        "ElementGetGround",          // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("空中ひねり")},  // mParent
        "Spin",          // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("地上ひねり")},  // mParent
        "SpinGround",    // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("アイスひねり")},  // mParent
        "IceSpin",         // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("アイスひねり移動")},  // mParent
        "IceSkateSpin",        // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("アイスひねり静止")},  // mParent
        "IceSpin",             // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("ハチスピン空中")},  // mParent
        "BeeSpin",           // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ハチスピン")},   // mParent
        "BeeSpinGround",  // fileName
        0,                // animationHash
        0,                // fileHash
    },
    {
        {CP932("アイスひねり空中")},  // mParent
        "IceSpinAir",          // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("スケートアクセルジャンプ")},  // mParent
        "IceJump",                     // fileName
        0,                             // animationHash
        0,                             // fileHash
    },
    {
        {CP932("スケートジャンプ2")},  // mParent
        "IceJump2",             // fileName
        0,                      // animationHash
        0,                      // fileHash
    },
    {
        {CP932("スケートジャンプ3")},  // mParent
        "IceJump3",             // fileName
        0,                      // animationHash
        0,                      // fileHash
    },
    {
        {CP932("スケート着地")},  // mParent
        "IceJumpLand",     // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("スケート静止着地")},  // mParent
        "IceJumpStopLand",     // fileName
        0,                     // animationHash
        0,                     // fileHash
    },
    {
        {CP932("サーフィンジャンプ")},  // mParent
        "SurfJump",              // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("サーフィンハイジャンプ")},  // mParent
        "SurfJumpHigh",              // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("サーフィン着地")},  // mParent
        "SurfLand",          // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("見る")},  // mParent
        "Watch",   // fileName
        0,         // animationHash
        0,         // fileHash
    },
    {
        {CP932("レース見る")},  // mParent
        "Watch",         // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("ステージインA")},   // mParent
        "StageStartGround",  // fileName
        0,                   // animationHash
        0,                   // fileHash
    },
    {
        {CP932("ステージインB")},    // mParent
        "LandScenarioStart",  // fileName
        0,                    // animationHash
        0,                    // fileHash
    },
    {
        {CP932("ウォークイン")},  // mParent
        "GoThrough",       // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("スピンゲット[デモ1]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[会話1]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[デモ2]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[会話2]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[デモ3]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[会話3]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[デモ4]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[デモ5]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[会話4]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[デモ6]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[デモ7]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("スピンゲット[デモ8]")},  // mParent
        "DemoGetPower",           // fileName
        0,                        // animationHash
        0,                        // fileHash
    },
    {
        {CP932("レース開始")},  // mParent
        "RaceStart",     // fileName
        0,               // animationHash
        0,               // fileHash
    },
    {
        {CP932("レースクラウチング開始")},  // mParent
        "RaceStartCrouch",           // fileName
        0,                           // animationHash
        0,                           // fileHash
    },
    {
        {CP932("ゴーストレース開始")},  // mParent
        "RaceStartGhost",        // fileName
        0,                       // animationHash
        0,                       // fileHash
    },
    {
        {CP932("ゴースト勝利")},  // mParent
        "WinGhost",        // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {CP932("ゴースト出現")},  // mParent
        "AppearGhost",     // fileName
        0,                 // animationHash
        0,                 // fileHash
    },
    {
        {""},  // mParent
        "",    // fileName
        0,     // animationHash
        0,     // fileHash
    },
};

XanimeGroupInfo marioAnimeTable[] = {
    {
        {CP932("基本")},        // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しゃがみ基本")},  // mParent
        1.00000000000f,    // 0x4
        0x2,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳基本")},    // mParent
        1.00000000000f,  // 0x4
        0x1e,            // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("テニス基本")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂左右ウエイト")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂前後ウエイト")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂左ウエイト")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂右ウエイト")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂前ウエイト")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂後ウエイト")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ラン")},        // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("歩行")},        // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("鈍行")},        // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("メタルダッシュ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ダッシュジャンプ")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("飛びすさりジャンプ")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ショートジャンプ")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("埋まり歩行")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("埋まりジャンプA")},  // mParent
        1.00000000000f,       // 0x4
        0x1,                  // 0x8
        0.00000000000f,       // 0xC
        0.00000000000f,       // 0x10
        0.00000000000f,       // 0x14
        0,                    // 0x18
        0,                    // 0x1C
        0,                    // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("埋まりジャンプB")},  // mParent
        1.00000000000f,       // 0x4
        0x1,                  // 0x8
        0.00000000000f,       // 0xC
        0.00000000000f,       // 0x10
        0.00000000000f,       // 0x14
        0,                    // 0x18
        0,                    // 0x1C
        0,                    // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("埋まり脱出ジャンプ")},  // mParent
        1.00000000000f,          // 0x4
        0x1,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("逆着地")},      // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しゃがみ")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ブレーキ")},    // mParent
        1.00000000000f,  // 0x4
        0x4,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ブレーキ滑り床")},  // mParent
        4.00000000000f,      // 0x4
        0x4,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ターンブレーキ")},  // mParent
        1.00000000000f,      // 0x4
        0,                   // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ターンブレーキ滑り床")},  // mParent
        3.00000000000f,            // 0x4
        0x4,                       // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("歩行制動ブレーキ")},  // mParent
        1.00000000000f,        // 0x4
        0x4,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷上慣性走行")},  // mParent
        1.00000000000f,    // 0x4
        0x10,              // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷上力行左")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷上力行右")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷上切替左")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷上切替右")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷上後行左")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷上後行右")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷上ターン")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("特殊ウエイト1A")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("特殊ウエイト1B")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("戦闘ウエイト")},  // mParent
        1.00000000000f,    // 0x4
        0x1e,              // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("匍匐前進")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ジャンプ")},    // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ジャンプB")},   // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ジャンプC")},   // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ターンジャンプ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("フリージャンプ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ジャンプふみ1")},  // mParent
        1.00000000000f,     // 0x4
        0x6,                // 0x8
        0.00000000000f,     // 0xC
        0.00000000000f,     // 0x10
        0.00000000000f,     // 0x14
        0,                  // 0x18
        0,                  // 0x1C
        0,                  // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ジャンプふみ2")},  // mParent
        1.00000000000f,     // 0x4
        0x6,                // 0x8
        0.00000000000f,     // 0xC
        0.00000000000f,     // 0x10
        0.00000000000f,     // 0x14
        0,                  // 0x18
        0,                  // 0x1C
        0,                  // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ジャンプふみ3")},  // mParent
        1.00000000000f,     // 0x4
        0x6,                // 0x8
        0.00000000000f,     // 0xC
        0.00000000000f,     // 0x10
        0.00000000000f,     // 0x14
        0,                  // 0x18
        0,                  // 0x1C
        0,                  // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("引き戻し")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("引き戻し着地")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ポッドワープ開始")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ポッドワープ終了")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スカイラブジャンプ")},  // mParent
        1.00000000000f,          // 0x4
        0,                       // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("空中一回転")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スケキヨ")},    // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スケキヨ脱出")},  // mParent
        1.00000000000f,    // 0x4
        0x1,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("埋まり")},      // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("埋まり脱出")},  // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパージャンプA")},  // mParent
        1.00000000000f,         // 0x4
        0x6,                    // 0x8
        0.00000000000f,         // 0xC
        0.00000000000f,         // 0x10
        0.00000000000f,         // 0x14
        0,                      // 0x18
        0,                      // 0x1C
        0,                      // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパージャンプB")},  // mParent
        1.00000000000f,         // 0x4
        0x6,                    // 0x8
        0.00000000000f,         // 0xC
        0.00000000000f,         // 0x10
        0.00000000000f,         // 0x14
        0,                      // 0x18
        0,                      // 0x1C
        0,                      // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパーふみジャンプA")},  // mParent
        1.00000000000f,             // 0x4
        0x6,                        // 0x8
        0.00000000000f,             // 0xC
        0.00000000000f,             // 0x10
        0.00000000000f,             // 0x14
        0,                          // 0x18
        0,                          // 0x1C
        0,                          // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパーふみジャンプB")},  // mParent
        1.00000000000f,             // 0x4
        0x6,                        // 0x8
        0.00000000000f,             // 0xC
        0.00000000000f,             // 0x10
        0.00000000000f,             // 0x14
        0,                          // 0x18
        0,                          // 0x1C
        0,                          // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパー移動A")},  // mParent
        1.00000000000f,     // 0x4
        0x6,                // 0x8
        0.00000000000f,     // 0xC
        0.00000000000f,     // 0x10
        0.00000000000f,     // 0x14
        0,                  // 0x18
        0,                  // 0x1C
        0,                  // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパー移動B")},  // mParent
        1.00000000000f,     // 0x4
        0x6,                // 0x8
        0.00000000000f,     // 0xC
        0.00000000000f,     // 0x10
        0.00000000000f,     // 0x14
        0,                  // 0x18
        0,                  // 0x1C
        0,                  // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパーハイジャンプA")},  // mParent
        1.00000000000f,             // 0x4
        0x1,                        // 0x8
        0.00000000000f,             // 0xC
        0.00000000000f,             // 0x10
        0.00000000000f,             // 0x14
        0,                          // 0x18
        0,                          // 0x1C
        0,                          // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパーハイジャンプB")},  // mParent
        1.00000000000f,             // 0x4
        0x1,                        // 0x8
        0.00000000000f,             // 0xC
        0.00000000000f,             // 0x10
        0.00000000000f,             // 0x14
        0,                          // 0x18
        0,                          // 0x1C
        0,                          // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパー壁ジャンプ")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパーヒップドロップ開始")},  // mParent
        1.00000000000f,                  // 0x4
        0x6,                             // 0x8
        0.00000000000f,                  // 0xC
        0.00000000000f,                  // 0x10
        0.00000000000f,                  // 0x14
        0,                               // 0x18
        0,                               // 0x1C
        0,                               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ホッパーヒップドロップ")},  // mParent
        1.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("幅とび")},      // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ヒップドロップ開始")},  // mParent
        1.00000000000f,          // 0x4
        0x2,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ヒップドロップ")},  // mParent
        1.00000000000f,      // 0x4
        0,                   // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ヒップドロップ着地")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ヒップドロップ滑り")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ジャンプ順滑り")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ジャンプ逆滑り")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スリップアップ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("つかまりスリップアップ準備")},  // mParent
        1.00000000000f,                  // 0x4
        0x4,                             // 0x8
        0.00000000000f,                  // 0xC
        0.00000000000f,                  // 0x10
        0.00000000000f,                  // 0x14
        0,                               // 0x18
        0,                               // 0x1C
        0,                               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("つかまりスリップアップ")},  // mParent
        1.00000000000f,              // 0x4
        0x2,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンヒップドロップ開始")},  // mParent
        1.00000000000f,                // 0x4
        0x2,                           // 0x8
        0.00000000000f,                // 0xC
        0.00000000000f,                // 0x10
        0.00000000000f,                // 0x14
        0,                             // 0x18
        0,                             // 0x1C
        0,                             // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンヒップドロップ")},  // mParent
        1.00000000000f,            // 0x4
        0,                         // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンヒップドロップ着地")},  // mParent
        1.00000000000f,                // 0x4
        0x6,                           // 0x8
        0.00000000000f,                // 0xC
        0.00000000000f,                // 0x10
        0.00000000000f,                // 0x14
        0,                             // 0x18
        0,                             // 0x1C
        0,                             // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("落下")},        // mParent
        1.00000000000f,  // 0x4
        0xf,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("着地")},        // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("着地B")},       // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハード着地")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("すべり着地")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ショート着地")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ飛行中")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ飛行中無入力")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ壁くっつき")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ壁くっつき中")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチジャンプ")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ壁ジャンプ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ匍匐ウエイト")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ匍匐前進")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ匍匐着地")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチ花移動")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチヒップドロップ開始")},  // mParent
        1.00000000000f,              // 0x4
        0x2,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチヒップドロップ")},  // mParent
        1.00000000000f,          // 0x4
        0,                       // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチヒップドロップ着地")},  // mParent
        1.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチヒップドロップ壁着地")},  // mParent
        1.00000000000f,                // 0x4
        0x6,                           // 0x8
        0.00000000000f,                // 0xC
        0.00000000000f,                // 0x10
        0.00000000000f,                // 0x14
        0,                             // 0x18
        0,                             // 0x1C
        0,                             // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("フーファイター飛行開始")},  // mParent
        1.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("フーファイター飛行")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("フーファイター飛行再開")},  // mParent
        1.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("フーファイター静止")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("フーファイター解除")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("フーファイター着地")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("フーファイタースピン")},  // mParent
        1.00000000000f,            // 0x4
        0x6,                       // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("泥低速歩行")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("泥高速歩行")},  // mParent
        2.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁くっつき")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁すべり")},    // mParent
        1.00000000000f,  // 0x4
        0xa,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁ジャンプ")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁押し")},      // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁左歩き")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁右歩き")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁ウエイト")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("前壁ウエイト")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁ヒット")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁ヒット着地")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("壁はじき")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ダメージ")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ダメージ着地")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ダメージウエイト")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ノーマルウエイト")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷結")},        // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("氷結解除")},    // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しびれ")},      // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しびれ回復")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("コインゲット")},  // mParent
        1.00000000000f,    // 0x4
        0x1,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("パンチ")},      // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("空パンチ")},    // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("左パンチ")},    // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("左空パンチ")},  // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ランニングキック")},  // mParent
        1.00000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("アッパーパンチ")},  // mParent
        1.00000000000f,      // 0x4
        0x3,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しゃがみアッパー")},  // mParent
        1.00000000000f,        // 0x4
        0x3,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サマーソルト")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ゲームオーバー")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("つぶれ")},      // mParent
        0.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("つぶれ復帰")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("つぶれ解除")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("空転")},        // mParent
        4.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂すべり上向きうつぶせ")},  // mParent
        1.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂すべり下向きあおむけ")},  // mParent
        2.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("向かい風走り")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("向かい風ふんばり")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サーフィン")},  // mParent
        1.00000000000f,  // 0x4
        0xf,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サーフィン（加速）")},  // mParent
        1.00000000000f,          // 0x4
        0xf,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サーフィン傾き開始")},  // mParent
        1.00000000000f,          // 0x4
        0x4,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サーフィン傾き開始（加速）")},  // mParent
        1.00000000000f,                  // 0x4
        0x4,                             // 0x8
        0.00000000000f,                  // 0xC
        0.00000000000f,                  // 0x10
        0.00000000000f,                  // 0x14
        0,                               // 0x18
        0,                               // 0x1C
        0,                               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サーフィンジャンプ")},  // mParent
        1.00000000000f,          // 0x4
        0x1,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サーフィンハイジャンプ")},  // mParent
        1.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サーフィン落下")},  // mParent
        1.00000000000f,      // 0x4
        0xf,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("サーフィン着地")},  // mParent
        1.00000000000f,      // 0x4
        0x1,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("タマコロ移動")},  // mParent
        1.00000000000f,    // 0x4
        0xf,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("タマコロしゃがみ")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スライダー尻")},  // mParent
        2.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("その場足踏み")},  // mParent
        1.00000000000f,    // 0x4
        0x2,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("その場足踏み上半身")},  // mParent
        1.00000000000f,          // 0x4
        0x4,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳一掻き")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水上一掻き")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳潜り")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ジャンプダイブ")},  // mParent
        1.00000000000f,          // 0x4
        0x1,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ジャンプダイブ回転")},  // mParent
        1.00000000000f,              // 0x4
        0x1,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳上昇呼吸")},  // mParent
        1.00000000000f,    // 0x4
        0x10,              // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳水面初期移動")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水上スピン")},  // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳スピン")},  // mParent
        1.00000000000f,  // 0x4
        0x1,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳スピン移動")},  // mParent
        1.00000000000f,      // 0x4
        0x1,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水上スピン移動")},  // mParent
        1.00000000000f,      // 0x4
        0x1,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ダメージ")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ダメージ亀")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ダメージ中")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水上ダメージ中")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水上ダメージ着水")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ダウン")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ターン左")},  // mParent
        1.00000000000f,    // 0x4
        0x14,              // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ターン右")},  // mParent
        1.00000000000f,    // 0x4
        0x14,              // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ターン下")},  // mParent
        1.00000000000f,    // 0x4
        0x14,              // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ウエイト")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ジェット")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ジェット開始")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ジェット終了")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳亀投げ")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳壁ヒット")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳ジェット壁ターン")},  // mParent
        1.00000000000f,            // 0x4
        0x6,                       // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳陸うちあげ")},  // mParent
        1.00000000000f,      // 0x4
        0x10,                // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("飛び込み準備")},  // mParent
        1.00000000000f,    // 0x4
        0x14,              // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("飛び込みジャンプ")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("後方飛び込みジャンプ")},  // mParent
        1.00000000000f,            // 0x4
        0x6,                       // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("飛び込み失敗着地")},  // mParent
        1.00000000000f,        // 0x4
        0xf,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("飛び込み失敗回転着地")},  // mParent
        1.00000000000f,            // 0x4
        0x6,                       // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水泳スピンジャンプ")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("リングダッシュ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("リングダッシュ準備")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("カメ持ちリング準備")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("カメ持ちリング")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("着地C")},       // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("着地ターン")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("着地幅とび")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しゃがみ開始")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しゃがみ終了")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂すべり上向き終了")},  // mParent
        1.00000000000f,          // 0x4
        0x4,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("坂すべり下向き終了")},  // mParent
        1.00000000000f,          // 0x4
        0x4,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ひろい")},      // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ひろい空中")},  // mParent
        1.00000000000f,  // 0x4
        0x3,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ひろいクイック")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ひろいウエイト")},  // mParent
        1.00000000000f,      // 0x4
        0,                   // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("カブ抜き")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("カブウエイト")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハンマー投げ回転中")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハンマー投げリリース")},  // mParent
        1.00000000000f,            // 0x4
        0x6,                       // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("投げ")},        // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ファイア投げ")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("両手投げ")},    // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ファイアスピン")},  // mParent
        1.00000000000f,      // 0x4
        0,                   // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ファイアスピン空中")},  // mParent
        1.00000000000f,          // 0x4
        0,                       // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("崖つかまり開始")},  // mParent
        1.00000000000f,      // 0x4
        0x5,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("崖つかまり中")},  // mParent
        1.00000000000f,    // 0x4
        0x5,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("崖つかまり終了")},  // mParent
        1.00000000000f,      // 0x4
        0x4,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("崖つかまり終了坂")},  // mParent
        1.00000000000f,        // 0x4
        0x4,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("崖つかまり降り")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("崖ふんばり")},  // mParent
        1.00000000000f,  // 0x4
        0x2,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しゃがみジャンプ")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しゃがみジャンプ着地")},  // mParent
        1.00000000000f,            // 0x4
        0x6,                       // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("しゃがみ歩き")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("がんばり走り")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("腹ばいジャンプ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("尻滑りジャンプ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("前方小ダメージ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("後方小ダメージ")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("強制ジャンプ")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ヘリコプタージャンプ")},  // mParent
        1.00000000000f,            // 0x4
        0x6,                       // 0x8
        0.00000000000f,            // 0xC
        0.00000000000f,            // 0x10
        0.00000000000f,            // 0x14
        0,                         // 0x18
        0,                         // 0x1C
        0,                         // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("中ダメージ")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("中ダメージ空中")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("中ダメージ着地")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("中後ダメージ")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("中後ダメージ空中")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("中後ダメージ着地")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ノーダメージ")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("炎ダメージ")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("炎のランナー")},  // mParent
        8.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ファイアラン前兆")},  // mParent
        1.00000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("電気ダメージ")},  // mParent
        1.00000000000f,    // 0x4
        0x1,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("電気ダメージ終了")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ファイアダンス")},  // mParent
        6.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("後転ふっとび")},  // mParent
        1.00000000000f,    // 0x4
        0x2,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("後転ふっとび空中")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("後転ふっとび着地")},  // mParent
        1.00000000000f,        // 0x4
        0x2,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("座りダウン")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("感電ダウン")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("炎ダウン")},    // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("仰向けダウン")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("俯せダウン")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("奈落ダウン")},  // mParent
        1.00000000000f,  // 0x4
        0x14,            // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ブラックホール落下")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("レース負け")},  // mParent
        1.00000000000f,  // 0x4
        0xf,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("水中レース負け")},  // mParent
        1.00000000000f,      // 0x4
        0xf,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("埋まりダウン")},  // mParent
        1.00000000000f,    // 0x4
        0x1e,              // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("つぶれダウン")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("はねとばされ")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("はねとばされ終了")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("テニスショット左")},  // mParent
        1.25000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("テニスショット右")},  // mParent
        1.25000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("テニスショット中")},  // mParent
        1.00000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("テニスショット空")},  // mParent
        1.25000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("テニスウエイト")},  // mParent
        1.00000000000f,      // 0x4
        0x6,                 // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("エレメントゲット")},  // mParent
        1.00000000000f,        // 0x4
        0x6,                   // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("エレメントゲット接地中")},  // mParent
        1.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("空中ひねり")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("地上ひねり")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチスピン")},  // mParent
        1.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ハチスピン空中")},  // mParent
        1.00000000000f,      // 0x4
        0,                   // 0x8
        0.00000000000f,      // 0xC
        0.00000000000f,      // 0x10
        0.00000000000f,      // 0x14
        0,                   // 0x18
        0,                   // 0x1C
        0,                   // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("アイスひねり")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("アイスひねり移動")},  // mParent
        1.00000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("アイスひねり静止")},  // mParent
        1.00000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("アイスひねり空中")},  // mParent
        1.00000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スケートアクセルジャンプ")},  // mParent
        1.00000000000f,                // 0x4
        0,                             // 0x8
        0.00000000000f,                // 0xC
        0.00000000000f,                // 0x10
        0.00000000000f,                // 0x14
        0,                             // 0x18
        0,                             // 0x1C
        0,                             // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スケートジャンプ2")},  // mParent
        1.00000000000f,         // 0x4
        0,                      // 0x8
        0.00000000000f,         // 0xC
        0.00000000000f,         // 0x10
        0.00000000000f,         // 0x14
        0,                      // 0x18
        0,                      // 0x1C
        0,                      // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スケートジャンプ3")},  // mParent
        1.00000000000f,         // 0x4
        0,                      // 0x8
        0.00000000000f,         // 0xC
        0.00000000000f,         // 0x10
        0.00000000000f,         // 0x14
        0,                      // 0x18
        0,                      // 0x1C
        0,                      // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スケート着地")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スケート静止着地")},  // mParent
        1.00000000000f,        // 0x4
        0,                     // 0x8
        0.00000000000f,        // 0xC
        0.00000000000f,        // 0x10
        0.00000000000f,        // 0x14
        0,                     // 0x18
        0,                     // 0x1C
        0,                     // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("見る")},        // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("レース見る")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("カリカリ限界")},  // mParent
        1.00000000000f,    // 0x4
        0x6,               // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ステージインA")},  // mParent
        1.00000000000f,     // 0x4
        0,                  // 0x8
        0.00000000000f,     // 0xC
        0.00000000000f,     // 0x10
        0.00000000000f,     // 0x14
        0,                  // 0x18
        0,                  // 0x1C
        0,                  // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ステージインB")},  // mParent
        1.00000000000f,     // 0x4
        0,                  // 0x8
        0.00000000000f,     // 0xC
        0.00000000000f,     // 0x10
        0.00000000000f,     // 0x14
        0,                  // 0x18
        0,                  // 0x1C
        0,                  // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ウォークイン")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[デモ1]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[会話1]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[デモ2]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[会話2]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[デモ3]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[会話3]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[デモ4]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[デモ5]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[会話4]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[デモ6]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[デモ7]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("スピンゲット[デモ8]")},  // mParent
        1.00000000000f,           // 0x4
        0,                        // 0x8
        0.00000000000f,           // 0xC
        0.00000000000f,           // 0x10
        0.00000000000f,           // 0x14
        0,                        // 0x18
        0,                        // 0x1C
        0,                        // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("レース開始")},  // mParent
        1.00000000000f,  // 0x4
        0x6,             // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("レースクラウチング開始")},  // mParent
        1.00000000000f,              // 0x4
        0x6,                         // 0x8
        0.00000000000f,              // 0xC
        0.00000000000f,              // 0x10
        0.00000000000f,              // 0x14
        0,                           // 0x18
        0,                           // 0x1C
        0,                           // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ゴーストレース開始")},  // mParent
        1.00000000000f,          // 0x4
        0x6,                     // 0x8
        0.00000000000f,          // 0xC
        0.00000000000f,          // 0x10
        0.00000000000f,          // 0x14
        0,                       // 0x18
        0,                       // 0x1C
        0,                       // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ゴースト勝利")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {CP932("ゴースト出現")},  // mParent
        1.00000000000f,    // 0x4
        0,                 // 0x8
        0.00000000000f,    // 0xC
        0.00000000000f,    // 0x10
        0.00000000000f,    // 0x14
        0,                 // 0x18
        0,                 // 0x1C
        0,                 // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
    {
        {""},            // mParent
        0.00000000000f,  // 0x4
        0,               // 0x8
        0.00000000000f,  // 0xC
        0.00000000000f,  // 0x10
        0.00000000000f,  // 0x14
        0,               // 0x18
        0,               // 0x1C
        0,               // 0x1D
        {
            0,
            0,
            0,
            0,
        },  // 0x20
        {
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
            0.00000000000f,
        },        // 0x30
        0,        // 0x40
        0,        // 0x44
        nullptr,  // 0x48
    },
};

XanimeAuxInfo marioAnimeAuxTable[] = {{""}};

XanimeOfsInfo marioAnimeOfsTable[] = {
    {
        {CP932("ダメージ")},    // mParent
        0.00000000000f,  // 0x4
        10.0000000000f,  // 0x8
        10.0000000000f,  // 0xC
        0x1,             // 0x10
    },
    {
        {CP932("坂すべり下向きあおむけ")},  // mParent
        0.00000000000f,              // 0x4
        2.00000000000f,              // 0x8
        2.00000000000f,              // 0xC
        0x1,                         // 0x10
    },
    {
        {CP932("坂すべり上向きうつぶせ")},  // mParent
        0.00000000000f,              // 0x4
        2.00000000000f,              // 0x8
        2.00000000000f,              // 0xC
        0x1,                         // 0x10
    },
    {
        {CP932("ターンブレーキ")},  // mParent
        0.00000000000f,      // 0x4
        20.0000000000f,      // 0x8
        20.0000000000f,      // 0xC
        0x1,                 // 0x10
    },
    {
        {CP932("ジャンプ")},    // mParent
        0.00000000000f,  // 0x4
        24.0000000000f,  // 0x8
        24.0000000000f,  // 0xC
        0x1,             // 0x10
    },
    {
        {CP932("壁ヒット")},    // mParent
        0.00000000000f,  // 0x4
        25.0000000000f,  // 0x8
        25.0000000000f,  // 0xC
        0x1,             // 0x10
    },
    {
        {CP932("壁ヒット着地")},  // mParent
        0.00000000000f,    // 0x4
        67.0000000000f,    // 0x8
        0.00000000000f,    // 0xC
        0,                 // 0x10
    },
    {
        {CP932("カブ抜き")},    // mParent
        0.00000000000f,  // 0x4
        49.0000000000f,  // 0x8
        49.0000000000f,  // 0xC
        0x1,             // 0x10
    },
    {
        {CP932("落下")},        // mParent
        0.00000000000f,  // 0x4
        29.0000000000f,  // 0x8
        29.0000000000f,  // 0xC
        0x1,             // 0x10
    },
    {
        {CP932("ヒップドロップ開始")},  // mParent
        0.00000000000f,          // 0x4
        13.0000000000f,          // 0x8
        13.0000000000f,          // 0xC
        0x1,                     // 0x10
    },
    {
        {CP932("ヒップドロップ")},  // mParent
        0.00000000000f,      // 0x4
        9.00000000000f,      // 0x8
        9.00000000000f,      // 0xC
        0x1,                 // 0x10
    },
    {
        {CP932("スピンヒップドロップ")},  // mParent
        0.00000000000f,            // 0x4
        19.0000000000f,            // 0x8
        19.0000000000f,            // 0xC
        0x1,                       // 0x10
    },
    {
        {CP932("ハチヒップドロップ壁着地")},  // mParent
        0.00000000000f,                // 0x4
        20.0000000000f,                // 0x8
        20.0000000000f,                // 0xC
        0,                             // 0x10
    },
    {
        {CP932("ホッパーふみジャンプA")},  // mParent
        10.0000000000f,             // 0x4
        50.0000000000f,             // 0x8
        0.00000000000f,             // 0xC
        0x1,                        // 0x10
    },
    {
        {CP932("ホッパーふみジャンプB")},  // mParent
        10.0000000000f,             // 0x4
        50.0000000000f,             // 0x8
        0.00000000000f,             // 0xC
        0x1,                        // 0x10
    },
    {
        {CP932("テニスショット左")},  // mParent
        4.00000000000f,        // 0x4
        26.0000000000f,        // 0x8
        26.0000000000f,        // 0xC
        0,                     // 0x10
    },
    {
        {CP932("テニスショット右")},  // mParent
        4.00000000000f,        // 0x4
        26.0000000000f,        // 0x8
        26.0000000000f,        // 0xC
        0,                     // 0x10
    },
    {
        {CP932("テニスショット中")},  // mParent
        3.00000000000f,        // 0x4
        38.0000000000f,        // 0x8
        38.0000000000f,        // 0xC
        0,                     // 0x10
    },
    {
        {CP932("テニスショット空")},  // mParent
        5.00000000000f,        // 0x4
        26.0000000000f,        // 0x8
        26.0000000000f,        // 0xC
        0,                     // 0x10
    },
    {
        {CP932("その場足踏み")},  // mParent
        0.00000000000f,    // 0x4
        17.0000000000f,    // 0x8
        8.00000000000f,    // 0xC
        0,                 // 0x10
    },
    {
        {CP932("ブレーキ")},    // mParent
        0.00000000000f,  // 0x4
        2.00000000000f,  // 0x8
        0.00000000000f,  // 0xC
        0x1,             // 0x10
    },
    {
        {CP932("ショート着地")},  // mParent
        0.00000000000f,    // 0x4
        5.00000000000f,    // 0x8
        0.00000000000f,    // 0xC
        0,                 // 0x10
    },
    {
        {CP932("アッパーパンチ")},  // mParent
        22.0000000000f,      // 0x4
        23.0000000000f,      // 0x8
        0.00000000000f,      // 0xC
        0x1,                 // 0x10
    },
    {
        {CP932("しゃがみアッパー")},  // mParent
        0.00000000000f,        // 0x4
        6.00000000000f,        // 0x8
        0.00000000000f,        // 0xC
        0,                     // 0x10
    },
    {
        {CP932("スケキヨ")},    // mParent
        0.00000000000f,  // 0x4
        59.0000000000f,  // 0x8
        30.0000000000f,  // 0xC
        0x2,             // 0x10
    },
    {
        {CP932("スケキヨ脱出")},  // mParent
        60.0000000000f,    // 0x4
        120.000000000f,    // 0x8
        0.00000000000f,    // 0xC
        0,                 // 0x10
    },
    {
        {CP932("特殊ウエイト1A")},  // mParent
        0.00000000000f,      // 0x4
        494.000000000f,      // 0x8
        375.000000000f,      // 0xC
        0x2,                 // 0x10
    },
    {
        {CP932("特殊ウエイト1B")},  // mParent
        0.00000000000f,      // 0x4
        254.000000000f,      // 0x8
        135.000000000f,      // 0xC
        0x2,                 // 0x10
    },
    {
        {CP932("アイスひねり")},  // mParent
        0.00000000000f,    // 0x4
        35.0000000000f,    // 0x8
        35.0000000000f,    // 0xC
        0,                 // 0x10
    },
    {
        {CP932("ファイアスピン空中")},  // mParent
        0.00000000000f,          // 0x4
        30.0000000000f,          // 0x8
        30.0000000000f,          // 0xC
        0,                       // 0x10
    },
    {
        {CP932("崖つかまり終了坂")},  // mParent
        0.00000000000f,        // 0x4
        27.0000000000f,        // 0x8
        27.0000000000f,        // 0xC
        0,                     // 0x10
    },
    {
        {CP932("前壁ウエイト")},  // mParent
        57.0000000000f,    // 0x4
        59.0000000000f,    // 0x8
        57.0000000000f,    // 0xC
        0x1,               // 0x10
    },
    {
        {CP932("フーファイター静止")},  // mParent
        0.00000000000f,          // 0x4
        159.000000000f,          // 0x8
        40.0000000000f,          // 0xC
        0x2,                     // 0x10
    },
    {
        {CP932("ひろいクイック")},  // mParent
        0.00000000000f,      // 0x4
        26.0000000000f,      // 0x8
        0.00000000000f,      // 0xC
        0x1,                 // 0x10
    },
    {
        {""},            // mParent
        0.00000000000f,  // 0x4
        0.00000000000f,  // 0x8
        0.00000000000f,  // 0xC
        0,               // 0x10
    },
};

extern XanimeSwapTable luigiAnimeSwapTable[];
