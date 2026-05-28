target("smg-pc-game")
    set_kind("static")
    add_files("**.cpp")
    add_files("../camera/**.cpp")
    add_files("../layout/**.cpp")
    add_files("../resource/**.cpp")
    add_files("../runtime/**.cpp")
    add_files("../scene/**.cpp")
    add_files {
        "../render/effects/EffectResource.cpp",
        "../render/GXFrameBuffer.cpp",
        "../render/GXState.cpp",
        "../render/J3dAnimation.cpp",
        "../render/J3dMaterialRuntime.cpp",
        "../render/J3dMatrix.cpp",
        "../render/J3dModel.cpp",
        "../render/J3dModelRenderer.cpp",
        "../render/J3dTexture.cpp",
        "../render/JMathTrig.cpp",
        "../render/light/LightData.cpp",
        "../render/live_actor/LiveActorModel.cpp"
    }
    add_headerfiles("**.hpp")
    add_headerfiles("../camera/**.hpp")
    add_headerfiles("../layout/**.hpp")
    add_headerfiles("../resource/**.hpp")
    add_headerfiles("../runtime/**.hpp")
    add_headerfiles("../scene/**.hpp")
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
        "../render/live_actor/LiveActorModel.hpp"
    }
    add_includedirs("../", { public = true })
    add_deps {
        "smg-pc-common",
        "smg-pc-render"
    }
