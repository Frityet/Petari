target("smg-pc-game")
    set_kind("static")
    add_cxxflags("-Wno-register", {force = true})
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), { force = true })
    add_files("**.cpp")
    -- Retail XanimeCore uses unfused scalar arithmetic; its paired SDK calls
    -- preserve their explicit fused instructions in the compatibility layer.
    add_files("Animation/XanimeCore.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Animation/XanimePlayer.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("LiveActor/Binder.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Map/CollisionCategorizedKeeper.cpp", {cxxflags = "-ffp-contract=off"})
    -- The exact Player constructor closure is mirrored under src/Game/Player,
    -- but only MarioHolder has its native provider closure in production.
    -- Xmake's broad-remove/explicit-re-add order drops the re-added object from
    -- the archive, so keep the provider-incomplete set explicit here.
    remove_files {
        "Player/DrawAdaptor.cpp",
        "Player/FireMarioBall.cpp",
        "Player/J3DModelX.cpp",
        "Player/JetTurtleShadow.cpp",
        "Player/MarineSnow.cpp",
        "Player/Mario.cpp",
        "Player/Mario2D.cpp",
        "Player/MarioAccess.cpp",
        "Player/MarioActor.cpp",
        "Player/MarioActorCamera.cpp",
        "Player/MarioActorClap.cpp",
        "Player/MarioActorDefensiveMsg.cpp",
        "Player/MarioActorDraw.cpp",
        "Player/MarioActorEye.cpp",
        "Player/MarioActorGameOver.cpp",
        "Player/MarioActorGravity.cpp",
        "Player/MarioActorHand.cpp",
        "Player/MarioActorInit.cpp",
        "Player/MarioActorMatrix.cpp",
        "Player/MarioActorMorph.cpp",
        "Player/MarioActorOffensiveMsg.cpp",
        "Player/MarioActorPad.cpp",
        "Player/MarioActorParts.cpp",
        "Player/MarioActorPunch.cpp",
        "Player/MarioActorRush.cpp",
        "Player/MarioActorRushMsg.cpp",
        "Player/MarioActorSensor.cpp",
        "Player/MarioActorShadow.cpp",
        "Player/MarioActorSpecialDraw.cpp",
        "Player/MarioActorTakeMsg.cpp",
        "Player/MarioActorWipe.cpp",
        "Player/MarioAnimationEfx.cpp",
        "Player/MarioAnimator.cpp",
        "Player/MarioBee.cpp",
        "Player/MarioBlown.cpp",
        "Player/MarioBump.cpp",
        "Player/MarioClimb.cpp",
        "Player/MarioCollision.cpp",
        "Player/MarioConst.cpp",
        "Player/MarioDamage.cpp",
        "Player/MarioDamageCrush.cpp",
        "Player/MarioDamageFreeze.cpp",
        "Player/MarioDamageParalyze.cpp",
        "Player/MarioDamageStun.cpp",
        "Player/MarioEffect.cpp",
        "Player/MarioEnforce.cpp",
        "Player/MarioFaint.cpp",
        "Player/MarioFlip.cpp",
        "Player/MarioFlow.cpp",
        "Player/MarioFoo.cpp",
        "Player/MarioFpView.cpp",
        "Player/MarioFrontStep.cpp",
        "Player/MarioHang.cpp",
        "Player/MarioInit.cpp",
        "Player/MarioJump.cpp",
        "Player/MarioMagic.cpp",
        "Player/MarioMapCode.cpp",
        "Player/MarioMessenger.cpp",
        "Player/MarioModule.cpp",
        "Player/MarioMove.cpp",
        "Player/MarioMove25D.cpp",
        "Player/MarioMove2D.cpp",
        "Player/MarioMoveSphere.cpp",
        "Player/MarioNullBck.cpp",
        "Player/MarioParts.cpp",
        "Player/MarioPress.cpp",
        "Player/MarioRabbit.cpp",
        "Player/MarioRecovery.cpp",
        "Player/MarioSearchLight.cpp",
        "Player/MarioShadow.cpp",
        "Player/MarioSideStep.cpp",
        "Player/MarioSkate.cpp",
        "Player/MarioSlider.cpp",
        "Player/MarioSlip.cpp",
        "Player/MarioSlope.cpp",
        "Player/MarioSound.cpp",
        "Player/MarioSpecial.cpp",
        "Player/MarioSpin.cpp",
        "Player/MarioState.cpp",
        "Player/MarioStep.cpp",
        "Player/MarioStick.cpp",
        "Player/MarioSukekiyo.cpp",
        "Player/MarioSwim.cpp",
        "Player/MarioSwimDamage.cpp",
        "Player/MarioTalk.cpp",
        "Player/MarioTask.cpp",
        "Player/MarioTeresa.cpp",
        "Player/MarioWait.cpp",
        "Player/MarioWalk.cpp",
        "Player/MarioWall.cpp",
        "Player/MarioWarp.cpp",
        "Player/MatrixControl.cpp",
        "Player/ModelHolder.cpp",
        "Player/RushEndInfo.cpp",
        "Player/TornadoMario.cpp",
    }
    -- The retail source explicitly narrows its opaque host pointer to u32.
    -- Clang accepts this legacy cast in its extension mode; -fpermissive is GCC-only.
    add_files("Gravity/PlanetGravityManager.cpp", {
        cxxflags = is_plat("macosx", "iphoneos") and "-fms-extensions" or "-fpermissive"
    })
    add_files("AudioLib/AudBgmSetting.cpp", {cxxflags = "-Wno-narrowing"})
    add_files("AudioLib/AudParams.cpp", {
        force = {
            cxxflags = "-include " .. path.join(os.projectdir(), "src/compat/AudParamsSourceCompat.hpp")
        }
    })
    add_files("GameAudio/AudStageBgmTable.cpp", {cxxflags = "-Wno-narrowing"})
    add_files("GameAudio/AudStageBgmWrap.cpp", {cxxflags = "-Wno-narrowing"})
    add_files("Map/FileSelectSky.cpp", {
        force = {
            cxxflags = "-include " .. path.join(os.projectdir(), "src/JSystem/JMath/JMATrigonometric.hpp")
        }
    })
    remove_files("AudioLib/AudBgmKeeper.cpp")
    remove_files("AudioLib/AudBgmMgr.cpp")
    remove_files("AudioLib/AudBgmRhythmStrategy.cpp")
    remove_files("AudioLib/AudFader.cpp")
    remove_files("AudioLib/AudTrackController.cpp")
    remove_files("AudioLib/AudWrap.cpp")
    remove_files("AreaObj/AreaObjContainer.cpp")
    remove_files("Screen/FileSelectInfo.cpp")
    remove_files("Screen/LayoutActor.cpp")
    remove_files("Screen/LayoutPaneCtrl.cpp")
    remove_files("Screen/SimpleLayout.cpp")
    remove_files("Screen/FullScreenBlur.cpp")
    remove_files("LiveActor/MaterialCtrl.cpp")
    remove_files("Map/FileSelectEffect.cpp")
    remove_files("Map/FileSelectFunc.cpp")
    remove_files("Map/FileSelectItem.cpp")
    remove_files("Map/FileSelector.cpp")
    remove_files("MapObj/StarPiece.cpp")
    remove_files("MapObj/StarPieceGroup.cpp")
    remove_files("NameObj/NameObjFactory.cpp")
    remove_files("Scene/SceneObjHolder.cpp")
    remove_files("System/StorySequenceExecutor.cpp")
    remove_files("System/GameDataFunction.cpp")
    remove_files("System/GameDataHolder.cpp")
    remove_files("System/SaveDataHandleSequence.cpp")
    remove_files("System/BinaryDataChunkHolder.cpp")
    remove_files("System/BinaryDataContentAccessor.cpp")
    remove_files("System/ConfigDataMisc.cpp")
    remove_files("System/GameDataTemporaryInGalaxy.cpp")
    remove_files("System/SysConfigFile.cpp")
    remove_files("Util/ActorSensorUtil.cpp")
    remove_files("Util/SequenceUtil.cpp")
    remove_files("Util/CameraUtil.cpp")
    remove_files("Util/GamePadUtil.cpp")
    remove_files("Util/GravityUtil.cpp")
    remove_files("Util/DemoUtil.cpp")
    remove_files("Util/EventUtil.cpp")
    remove_files("Util/FixedPosition.cpp")
    remove_files("Util/ActorMovementUtil.cpp")
    remove_files("Util/ActorShadowUtil.cpp")
    remove_files("Util/LiveActorUtil.cpp")
    remove_files("Util/JointController.cpp")
    remove_files("Util/MapUtil.cpp")
    remove_files("Util/MathUtil.cpp")
    remove_files("Util/MessageUtil.cpp")
    remove_files("Util/PlayerUtil.cpp")
    remove_files("Util/SceneUtil.cpp")
    remove_files("Util/SoundUtil.cpp")
    remove_files("Util/StringUtil.cpp")
    remove_files("Util/SystemUtil.cpp")
    remove_files("NPC/MiiFacePartsHolder.cpp")
    remove_files("NPC/MiiFaceParts.cpp")
    remove_files("NPC/MiiFaceRecipe.cpp")
    remove_files("NPC/NPCActor.cpp")
    add_files("../camera/**.cpp")
    add_files("../layout/**.cpp")
    add_files("../resource/**.cpp")
    add_files("../runtime/**.cpp")
    add_files("../scene/**.cpp")
    add_files("../compat/**.cpp")
    -- Original paired-single helpers make fused and rounded operations explicit.
    add_files("../compat/GameMathCompat.cpp", {cxxflags = "-ffp-contract=off"})
    add_files {
        "../render/effects/JpcBillboard.cpp",
        "../render/effects/EffectResource.cpp",
        "../render/GXState.cpp",
        "../render/J3dAnimation.cpp",
        "../render/J3dMaterialRuntime.cpp",
        "../render/J3dMatrix.cpp",
        "../render/J3dModel.cpp",
        "../render/J3dModelRenderer.cpp",
        "../render/J3dTexture.cpp",
        "../render/JMathTrig.cpp",
        "../JSystem/JGeometry/TQuat.cpp",
        "../JSystem/JGeometry/TMatrix.cpp",
        "../JSystem/JMath/JMATrigonometricTable.cpp",
        "../render/light/LightData.cpp",
        "../render/live_actor/LiveActorModel.cpp"
    }
    add_headerfiles("**.hpp")
    add_headerfiles("../camera/**.hpp")
    add_headerfiles("../layout/**.hpp")
    add_headerfiles("../resource/**.hpp")
    add_headerfiles("../runtime/**.hpp")
    add_headerfiles("../scene/**.hpp")
    add_headerfiles("../compat/**.hpp")
    add_headerfiles {
        "../render/effects/JpcBillboard.hpp",
        "../render/effects/EffectResource.hpp",
        "../render/GXState.hpp",
        "../render/J3dAnimation.hpp",
        "../render/J3dMaterialRuntime.hpp",
        "../render/J3dMatrix.hpp",
        "../render/J3dModel.hpp",
        "../render/J3dModelRenderer.hpp",
        "../render/J3dTexture.hpp",
        "../render/JMathTrig.hpp",
        "../render/light/LightData.hpp",
        "../render/live_actor/LiveActorModel.hpp"
    }
    add_includedirs("../", { public = true })
    add_deps {
        "smg-pc-common",
        "smg-pc-render",
        "aurora-nw4r",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-ms",
        "aurora-mtx",
        "aurora-os",
        "aurora-pad",
        "aurora-vi"
    }
