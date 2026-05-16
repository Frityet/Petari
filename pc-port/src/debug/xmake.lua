target("smg-pc-title-probe")
    set_kind("binary")
    set_default(false)
    add_files("TitleSequenceProbe.cpp")
    add_deps {
        "smg-pc-game",
        "smg-pc-common"
    }

target("smg-pc-tpl-dump")
    set_kind("binary")
    set_default(false)
    add_files("TplDump.cpp")
    add_deps {
        "smg-pc-game",
        "smg-pc-render"
    }

target("smg-pc-j3d-texture-dump")
    set_kind("binary")
    set_default(false)
    add_files("J3dTextureDump.cpp")
    add_deps {
        "smg-pc-game",
        "smg-pc-render"
    }

target("smg-pc-j3d-model-probe")
    set_kind("binary")
    set_default(false)
    add_files("J3dModelProbe.cpp")
    add_deps {
        "smg-pc-game",
        "smg-pc-render"
    }

target("smg-pc-j3d-animation-probe")
    set_kind("binary")
    set_default(false)
    add_files("J3dAnimationProbe.cpp")
    add_deps {
        "smg-pc-game"
    }

target("smg-pc-layout-probe")
    set_kind("binary")
    set_default(false)
    add_files("LayoutProbe.cpp")
    add_deps {
        "smg-pc-game"
    }

target("smg-pc-rarc-probe")
    set_kind("binary")
    set_default(false)
    add_files("RarcProbe.cpp")
    add_deps {
        "smg-pc-game"
    }

target("smg-pc-brfnt-probe")
    set_kind("binary")
    set_default(false)
    add_files("BrfntProbe.cpp")
    add_deps {
        "smg-pc-game",
        "smg-pc-render"
    }

target("smg-pc-bcsv-probe")
    set_kind("binary")
    set_default(false)
    add_files("BcsvProbe.cpp")
    add_deps {
        "smg-pc-game"
    }

target("smg-pc-visual-diff")
    set_kind("binary")
    set_default(false)
    add_files("VisualDiff.cpp")
    add_files("$(projectdir)/dolphin/Externals/libspng/libspng/spng/spng.c")
    add_includedirs("$(projectdir)/dolphin/Externals/libspng/libspng/spng")
    add_includedirs("$(projectdir)/dolphin/Externals/zlib-ng")
    add_ldflags("-l:libz.so.1")
