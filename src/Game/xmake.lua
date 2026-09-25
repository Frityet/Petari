target("smg-pc-game")
    set_kind("static")
    add_cxxflags("-Wno-register", {force = true})
	add_cxxflags("-Wno-inconsistent-missing-override")
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), { force = true })
    add_cxxflags("-fno-builtin-sprintf", "-fno-builtin-snprintf", "-fno-builtin-vsprintf", "-fno-builtin-vsnprintf", {force = true})
    add_files("**.cpp")
    -- Retail XanimeCore uses unfused scalar arithmetic; its paired SDK calls
    -- preserve their explicit fused instructions in the compatibility layer.
    add_files("Animation/XanimeCore.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Animation/XanimePlayer.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Camera/CameraContext.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("LiveActor/Binder.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("LiveActor/HitSensorInfo.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Map/CollisionCategorizedKeeper.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("AudioLib/AudBgmSetting.cpp", {cxxflags = "-Wno-narrowing"})
    add_files("GameAudio/AudStageBgmTable.cpp", {cxxflags = "-Wno-narrowing"})
    add_files("GameAudio/AudStageBgmWrap.cpp", {cxxflags = "-Wno-narrowing"})
    add_files("Map/FileSelectSky.cpp", {
        force = {
            cxxflags = "-include " .. path.join(os.projectdir(), "src/JSystem/JMath/JMATrigonometric.hpp")
        }
    })
    -- WiiConnect24 is unavailable on the native host; no IOS mail worker exists.
    remove_files("NWC24/NWC24SendThread.cpp")
    remove_files("AudioLib/AudBgm.cpp")
    remove_files("AudioLib/AudBgmKeeper.cpp")
    remove_files("AudioLib/AudBgmMgr.cpp")
    remove_files("AudioLib/AudBgmRhythmStrategy.cpp")
    remove_files("AudioLib/AudFader.cpp")
    remove_files("AudioLib/AudTrackController.cpp")
    remove_files("AudioLib/AudSystemVolumeController.cpp")
    remove_files("AudioLib/AudWrap.cpp")
    remove_files("Screen/FileSelectInfo.cpp")
    remove_files("Screen/LayoutActor.cpp")
    remove_files("Screen/LayoutPaneCtrl.cpp")
    remove_files("Screen/FullScreenBlur.cpp")
    remove_files("Map/FileSelectEffect.cpp")
    remove_files("Map/CollisionParts.cpp")
    remove_files("MapObj/DynamicCollisionObj.cpp")
    remove_files("Map/FileSelectItem.cpp")
    remove_files("Map/FileSelector.cpp")
    remove_files("NameObj/NameObjFactory.cpp")
    remove_files("Scene/SceneFunction.cpp")
    remove_files("Scene/SceneObjHolder.cpp")
    remove_files("System/GameDataFunction.cpp")
    remove_files("System/Language.cpp")
    remove_files("System/BinaryDataChunkHolder.cpp")
    remove_files("System/ConfigDataMisc.cpp")
    remove_files("System/SysConfigFile.cpp")
    remove_files("Util/ActorSensorUtil.cpp")
    remove_files("Util/FileUtil.cpp")
    remove_files("Util/ActorShadowUtil.cpp")
    remove_files("Util/LiveActorUtil.cpp")
    remove_files("Util/MapUtil.cpp")
    remove_files("Util/StarPointerUtil.cpp")
    remove_files("Util/SoundUtil.cpp")
    remove_files("Util/LightUtil.cpp")
    remove_files("Map/LightFunction.cpp")
    remove_files("Map/LightDirector.cpp")
    remove_files("NPC/MiiFacePartsHolder.cpp")
    remove_files("NPC/MiiFaceParts.cpp")
    remove_files("NPC/MiiFaceRecipe.cpp")
    add_files({"../nw4r/ut/ut_CharWriter.cpp", "../nw4r/ut/ut_TextWriterBase.cpp",
               "../nw4r/ut/ut_TagProcessorBase.cpp", "../nw4r/ut/ut_CharStrmReader.cpp",
               "../nw4r/ut/ut_Font.cpp"}, {cxxflags = "-ffp-contract=off"})
    add_files({"../nw4r/lyt/lyt_animation.cpp", "../nw4r/lyt/lyt_group.cpp",
               "../nw4r/lyt/lyt_layout.cpp", "../nw4r/lyt/lyt_arcResourceAccessor.cpp",
               "../nw4r/lyt/lyt_resourceAccessor.cpp", "../nw4r/lyt/lyt_common.cpp",
               "../nw4r/lyt/lyt_material.cpp", "../nw4r/lyt/lyt_texMap.cpp",
               "../nw4r/lyt/lyt_textBox.cpp", "../nw4r/lyt/lyt_picture.cpp",
               "../nw4r/lyt/lyt_window.cpp", "../nw4r/lyt/lyt_bounding.cpp",
               "../nw4r/lyt/lyt_drawInfo.cpp", "../nw4r/lyt/lyt_init.cpp",
               "../nw4r/math/math_triangular.cpp"},
              {cxxflags = "-ffp-contract=off"})
    add_files("../camera/**.cpp")
    add_files("../layout/**.cpp")
    add_files("../resource/**.cpp")
    add_files("../runtime/**.cpp")
    add_files("../scene/**.cpp")
    add_files("../compat/**.cpp")
    -- Its numeric fallback must call host libc outside the forced MSL aliases.
    remove_files("../compat/MslPrintfCompat.cpp")
    -- Original paired-single helpers make fused and rounded operations explicit.
    add_files("Util/MathUtil.cpp", {cxxflags = "-ffp-contract=off"})
    add_files({"Util/ActorMovementUtil.cpp", "../compat/OriginalMapQueries.cpp",
               "../compat/OriginalCollisionPartsCompat.cpp", "../compat/OriginalKCollisionCompat.cpp"},
              {cxxflags = "-ffp-contract=off"})
    add_files({"../JSystem/J3DGraphBase/J3DMaterial.cpp", "../JSystem/J3DGraphBase/J3DMatBlock.cpp",
               "../JSystem/J3DGraphBase/J3DTevs.cpp", "../JSystem/J3DGraphAnimator/J3DMaterialAnm.cpp",
               "../JSystem/J3DGraphAnimator/J3DMaterialAttach.cpp", "../JSystem/J3DGraphAnimator/J3DShapeTable.cpp"})
    add_files("../JSystem/J2DGraph/**.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("../JSystem/JParticle/**.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("../compat/OriginalJPADraw.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("../compat/OriginalJPAFields.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("../compat/OriginalJPAEmitterInit.cpp", {cxxflags = "-ffp-contract=off"})
    add_files {
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
        "../JSystem/JMath/JMATrigonometricTable.cpp",
        "../JSystem/JMath/random.cpp",
        "../render/light/LightData.cpp",
    }
    add_files("../JSystem/JGeometry/TMatrix.cpp", {cxxflags = "-ffp-contract=off"})
    add_files({
        "../JSystem/JKernel/JKRDisposer.cpp",
        "../JSystem/JKernel/JKRFileFinder.cpp",
        "../JSystem/JSupport/JSUList.cpp",
        "../JSystem/JUtility/JUTNameTab.cpp",
        "../JSystem/JAudio2/JAIAudible.cpp",
        "../JSystem/JAudio2/JAIAudience.cpp",
        "../JSystem/JAudio2/JAISound.cpp",
        "../JSystem/JAudio2/JAISoundChild.cpp",
        "../JSystem/JAudio2/JAISoundHandles.cpp",
        "../JSystem/JAudio2/JAISoundStarter.cpp",
        "../JSystem/JAudio2/JAIStream.cpp",
        "../JSystem/JAudio2/JAIStreamDataMgr.cpp",
        "../JSystem/JAudio2/JAIStreamMgr.cpp",
        "../JSystem/JAudio2/JASSoundParams.cpp",
    })
    add_files("../JSystem/J3DGraphBase/J3DPacket.cpp", "../nw4r/ut/ut_LinkList.cpp")
    add_files("../JSystem/JKernel/JKRThread.cpp")
    add_files("../JSystem/JAudio2/JAUSoundTable.cpp", "../JSystem/JGadget/hashcode.cpp")
    add_files("../JSystem/JUtility/JUTVideo.cpp", "../JSystem/JUtility/JUTXfb.cpp", "../JSystem/JUtility/JUTDirectPrint.cpp", "../JSystem/JUtility/JUTAssert.cpp", "../JSystem/JUtility/JUTConsole.cpp", "../JSystem/JUtility/JUTDbPrint.cpp", "../JSystem/JUtility/JUTFont.cpp", "../JSystem/JUtility/JUTPalette.cpp", "../JSystem/JGadget/linklist.cpp")
    add_files({"../JSystem/JKernel/JKRAram.cpp", "../JSystem/JKernel/JKRAramHeap.cpp",
               "../JSystem/JKernel/JKRAramBlock.cpp", "../JSystem/JKernel/JKRAramPiece.cpp",
               "../JSystem/JKernel/JKRAramStream.cpp", "../JSystem/JKernel/JKRDecomp.cpp",
               "../JSystem/JSupport/JSUFileStream.cpp"})
    add_headerfiles("**.hpp")
    add_headerfiles("../camera/**.hpp")
    add_headerfiles("../layout/**.hpp")
    add_headerfiles("../resource/**.hpp")
    add_headerfiles("../runtime/**.hpp")
    add_headerfiles("../scene/**.hpp")
    add_headerfiles("../compat/**.hpp")
    add_headerfiles {
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
        "aurora-vi",
        "aurora-thp"
    }
