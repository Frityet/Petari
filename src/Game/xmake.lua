target("smg-pc-game")
    set_kind("static")
    add_cxxflags("-Wno-register", {force = true})
	add_cxxflags("-Wno-inconsistent-missing-override")
    add_cxxflags("-include " .. path.join(os.projectdir(), "aurora/include/MSL_C/stdio.h"), { force = true })
    add_cxxflags("-fno-builtin-sprintf", "-fno-builtin-snprintf", "-fno-builtin-vsprintf", "-fno-builtin-vsnprintf", {force = true})
    add_files("**.cpp")
    -- Retail XanimeCore uses unfused scalar arithmetic; its paired SDK calls
    -- preserve their explicit fused instructions in the compatibility layer.
    add_files("Animation/XanimeCore.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Animation/XanimePlayer.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Camera/CameraContext.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Camera/CameraLocalUtil.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("LiveActor/Binder.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("LiveActor/HitSensorInfo.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Map/CollisionCategorizedKeeper.cpp", {cxxflags = "-ffp-contract=off"})
    add_files({"Screen/LayoutActor.cpp", "Screen/LayoutManager.cpp", "Screen/LayoutPaneCtrl.cpp", "Screen/LayoutGroupCtrl.cpp",
               "Screen/StarPointerDirector.cpp", "Util/StarPointerUtil.cpp"}, {cxxflags = "-ffp-contract=off"})
    add_files("AudioLib/**.cpp", {cxxflags = {"-ffp-contract=off", "-Wno-narrowing"}})
    add_files({"RhythmLib/AudMeObject.cpp", "RhythmLib/AudMeHandles.cpp", "RhythmLib/AudBgmTempoAdjuster.cpp"}, {cxxflags = "-ffp-contract=off"})
    add_files("GameAudio/AudStageBgmTable.cpp", {cxxflags = "-Wno-narrowing"})
    add_files("GameAudio/AudStageBgmWrap.cpp", {cxxflags = "-Wno-narrowing"})
    add_files("Map/FileSelectSky.cpp", {
        force = {
            cxxflags = "-include " .. path.join(os.projectdir(), "src/JSystem/JMath/JMATrigonometric.hpp")
        }
    })
    -- Original audio owners use JAudio sequencing and the native DSP boundary.
    -- WiiConnect24 is unavailable on the native host; no IOS mail worker exists.
    remove_files("NWC24/NWC24SendThread.cpp")
    add_files({"../nw4r/ut/ut_CharWriter.cpp", "../nw4r/ut/ut_TextWriterBase.cpp",
               "../nw4r/ut/ut_TagProcessorBase.cpp", "../nw4r/ut/ut_CharStrmReader.cpp",
               "../nw4r/ut/ut_Font.cpp", "../nw4r/ut/ut_ResFont.cpp",
               "../nw4r/ut/ut_ResFontBase.cpp", "../nw4r/ut/ut_binaryFileFormat.cpp"}, {cxxflags = "-ffp-contract=off"})
    add_files({"../nw4r/lyt/lyt_animation.cpp", "../nw4r/lyt/lyt_group.cpp",
               "../nw4r/lyt/lyt_layout.cpp", "../nw4r/lyt/lyt_arcResourceAccessor.cpp",
               "../nw4r/lyt/lyt_resourceAccessor.cpp", "../nw4r/lyt/lyt_common.cpp",
               "../nw4r/lyt/lyt_material.cpp", "../nw4r/lyt/lyt_texMap.cpp",
               "../nw4r/lyt/lyt_textBox.cpp", "../nw4r/lyt/lyt_picture.cpp",
               "../nw4r/lyt/lyt_window.cpp", "../nw4r/lyt/lyt_bounding.cpp",
               "../nw4r/lyt/lyt_drawInfo.cpp", "../nw4r/lyt/lyt_init.cpp", "../nw4r/lyt/lyt_pane.cpp",
               "../nw4r/math/math_triangular.cpp", "../nw4r/math/math_types.cpp"},
              {cxxflags = "-ffp-contract=off"})
    add_files("../nw4r/db/db_assert.cpp")
    add_files("../camera/**.cpp")
    add_files("../layout/**.cpp")
    add_files("../resource/**.cpp")
    add_files("../runtime/**.cpp")
    -- Original paired-single helpers make fused and rounded operations explicit.
    add_files("Util/MathUtil.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Util/LiveActorUtil.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Util/ActorShadowUtil.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Util/ActorShadowLocalUtil.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("Util/MtxUtil.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("System/Overwrite.cpp", {cxxflags = "-ffp-contract=off"})
    add_files({"Util/ActorMovementUtil.cpp", "Util/MapUtil.cpp",
               "Map/CollisionParts.cpp", "Map/HitInfo.cpp", "Map/KCollision.cpp", "Map/KCollisionPlus.cpp"},
              {cxxflags = "-ffp-contract=off"})
    add_files({"../JSystem/J3DGraphBase/J3DMaterial.cpp", "../JSystem/J3DGraphBase/J3DMatBlock.cpp",
               "../JSystem/J3DGraphBase/J3DTevs.cpp", "../JSystem/J3DGraphAnimator/J3DMaterialAnm.cpp",
               "../JSystem/J3DGraphAnimator/J3DMaterialAttach.cpp", "../JSystem/J3DGraphAnimator/J3DShapeTable.cpp"})
    add_files("../JSystem/J2DGraph/**.cpp", {cxxflags = "-ffp-contract=off"})
    add_files("../JSystem/JParticle/**.cpp", {cxxflags = "-ffp-contract=off"})
    add_files {
        "../render/effects/EffectResource.cpp",
        "../render/GXState.cpp",
        "../render/J3dAnimation.cpp",
        "../render/J3dMaterialRuntime.cpp",
        "../render/J3dMatrix.cpp",
        "../render/J3dModel.cpp",
        "../render/J3dTexture.cpp",
        "../render/JMathTrig.cpp",
        "../JSystem/JGeometry/TQuat.cpp",
        "../JSystem/JMath/JMATrigonometricTable.cpp",
        "../JSystem/JMath/random.cpp",
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
        "../JSystem/JAudio2/JAUSoundObject.cpp",
        "../JSystem/JAudio2/JAUSoundAnimator.cpp",
        "../JSystem/JAudio2/JAUSectionHeap.cpp",
        "../JSystem/JAudio2/JASHeapCtrl.cpp",
        "../JSystem/JAudio2/JASReport.cpp",
        "../JSystem/JAudio2/JASAramStream.cpp",
        "../JSystem/JAudio2/JASAiCtrl.cpp",
        "../JSystem/JAudio2/JAISeMgr.cpp",
        "../JSystem/JAudio2/JAISeqMgr.cpp",
        "../JSystem/JAudio2/JASTrack.cpp",
        "../JSystem/JAudio2/JASTrackPort.cpp",
        "../JSystem/JAudio2/JASSeqCtrl.cpp",
    })
    add_files("../JSystem/J3DGraphBase/J3DPacket.cpp", "../nw4r/ut/ut_LinkList.cpp")
    add_files("../JSystem/JKernel/JKRThread.cpp")
    add_files("../JSystem/J3DGraphAnimator/J3DAnimation.cpp",
              "../JSystem/J3DGraphLoader/J3DAnmLoader.cpp",
              "../JSystem/J3DGraphLoader/J3DModelLoader.cpp",
              "../JSystem/J3DGraphLoader/J3DMaterialFactory.cpp")
    add_files("../JSystem/J3DGraphBase/J3DDrawBuffer.cpp",
              "../JSystem/J3DGraphBase/J3DStruct.cpp",
              "../JSystem/J3DGraphBase/J3DGD.cpp", {cxxflags = "-ffp-contract=off"})
    add_files({
        "../JSystem/J3DGraphAnimator/J3DJoint.cpp",
        "../JSystem/J3DGraphAnimator/J3DJointTree.cpp",
        "../JSystem/J3DGraphAnimator/J3DModelData.cpp",
        "../JSystem/J3DGraphAnimator/J3DModel.cpp",
        "../JSystem/J3DGraphAnimator/J3DSkinDeform.cpp",
        "../JSystem/J3DGraphAnimator/J3DCluster.cpp",
        "../JSystem/J3DGraphBase/J3DTransform.cpp",
        "../JSystem/J3DGraphBase/J3DSys.cpp",
        "../JSystem/JMath/JMath.cpp",
        "../JSystem/JSupport/JSUInputStream.cpp",
        "../JSystem/JSupport/JSUOutputStream.cpp",
        "../JSystem/JSupport/JSUMemoryStream.cpp",
        "../JSystem/JKernel/JKRFileLoader.cpp",
        "../JSystem/JKernel/JKRArchivePub.cpp",
        "../JSystem/JKernel/JKRArchivePri.cpp",
        "../JSystem/JKernel/JKRMemArchive.cpp",
        "../JSystem/JKernel/JKRDvdAramRipper.cpp",
        "../JSystem/JKernel/JKRDvdFile.cpp",
        "../JSystem/JKernel/JKRDvdRipper.cpp",
    })
    add_files("../JSystem/JKernel/JKRHeap.cpp", "../JSystem/JKernel/JKRExpHeap.cpp",
              "../JSystem/JKernel/JKRSolidHeap.cpp", "../JSystem/JKernel/JKRUnitHeap.cpp")
    add_files("../JSystem/J3DGraphBase/J3DShape.cpp", "../JSystem/J3DGraphBase/J3DShapeDraw.cpp",
              "../JSystem/J3DGraphBase/J3DShapeMtx.cpp", "../JSystem/J3DGraphBase/J3DVertex.cpp",
              "../JSystem/J3DGraphLoader/J3DShapeFactory.cpp", "../JSystem/J3DGraphAnimator/J3DMtxBuffer.cpp")
    add_files("../JSystem/JAudio2/JAUSoundTable.cpp", "../JSystem/JGadget/hashcode.cpp")
    add_files({
        "../JSystem/JAudio2/JAISe.cpp",
        "../JSystem/JAudio2/JAISeq.cpp",
        "../JSystem/JAudio2/JAISeqDataMgr.cpp",
        "../JSystem/JAudio2/JAISoundInfo.cpp",
        "../JSystem/JAudio2/JAISoundParams.cpp",
        "../JSystem/JAudio2/JASBank.cpp",
        "../JSystem/JAudio2/JASBasicBank.cpp",
        "../JSystem/JAudio2/JASBasicInst.cpp",
        "../JSystem/JAudio2/JASBasicWaveBank.cpp",
        "../JSystem/JAudio2/JASBNKParser.cpp",
        "../JSystem/JAudio2/JASCallback.cpp",
        "../JSystem/JAudio2/JASChannel.cpp",
        "../JSystem/JAudio2/JASDSPChannel.cpp",
        "../JSystem/JAudio2/JASDriverIF.cpp",
        "../JSystem/JAudio2/JASDrumSet.cpp",
        "../JSystem/JAudio2/JASInstRand.cpp",
        "../JSystem/JAudio2/JASInstSense.cpp",
        "../JSystem/JAudio2/JASLfo.cpp",
        "../JSystem/JAudio2/JASOscillator.cpp",
        "../JSystem/JAudio2/JASRegisterParam.cpp",
        "../JSystem/JAudio2/JASSeqParser.cpp",
        "../JSystem/JAudio2/JASSeqReader.cpp",
        "../JSystem/JAudio2/JASSimpleWaveBank.cpp",
        "../JSystem/JAudio2/JASVoiceBank.cpp",
        "../JSystem/JAudio2/JASWSParser.cpp",
        "../JSystem/JAudio2/JAUAudience.cpp",
        "../JSystem/JAudio2/JAUAudioArcInterpreter.cpp",
        "../JSystem/JAudio2/JAUAudioArcLoader.cpp",
        "../JSystem/JAudio2/JAUBankTable.cpp",
        "../JSystem/JAudio2/JAUSeqCollection.cpp",
        "../JSystem/JAudio2/JAUSeqDataBlockMgr.cpp",
        "../JSystem/JAudio2/JAUSoundMgr.cpp",
        "../JSystem/JAudio2/JAUStdSoundInfo.cpp",
        "../JSystem/JAudio2/JAUStreamFileTable.cpp",
    }, {cxxflags = "-ffp-contract=off"})
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
    add_headerfiles {
        "../render/effects/EffectResource.hpp",
        "../render/GXState.hpp",
        "../render/J3dAnimation.hpp",
        "../render/J3dMaterialRuntime.hpp",
        "../render/J3dMatrix.hpp",
        "../render/J3dModel.hpp",
        "../render/J3dTexture.hpp",
        "../render/JMathTrig.hpp",
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

    add_files({
        "../JSystem/JAudio2/JASCalc.cpp",
        "../JSystem/JAudio2/JASResArcLoader.cpp",
        "../JSystem/JAudio2/JASWaveArcLoader.cpp",
        "../JSystem/JAudio2/JASTaskThread.cpp",
        "../JSystem/JAudio2/JASDvdThread.cpp",
        "../JSystem/JAudio2/JASCmdStack.cpp",
        "../JSystem/JAudio2/JASAudioReseter.cpp",
        "../JSystem/JAudio2/JASProbe.cpp",
    }, {cxxflags = "-ffp-contract=off"})

    add_files("../JSystem/JAudio2/JAUInitializer.cpp")

    add_files("../JSystem/JAudio2/JASDSPInterface.cpp")
