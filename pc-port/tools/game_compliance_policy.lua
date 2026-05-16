function policy()
return {
    version = 1,
    markers = {
        integration_begin = "SMGPC_INTEGRATION_BEGIN",
        integration_end = "SMGPC_INTEGRATION_END",
        stub_macro = "SMGPC_STUB",
    },
    scope = {
        mirror_root = "src/game/Game",
        hard_fail_root = "src/game",
        render_rewrite_exceptions = {
            "src/render/",
            "src/game/layout/",
        },
    },
    decomp_needed = {
    },
    files = {

    },
}
end
