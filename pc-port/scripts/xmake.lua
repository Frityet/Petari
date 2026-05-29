add_moduledirs("$(projectdir)/scripts")

task("validate-trace-ndjson")
    on_run(function()
        import("trace_ndjson")
        import("common")
        import("core.base.option")
        trace_ndjson.run({
            traces = option.get("contents") or {},
            require_emulator = option.get("require-emulator"),
            require_frame = option.get("require-frame") and tonumber(option.get("require-frame")) or nil,
            require_record_type = common.split_commas_or_table(option.get("require-record-type")),
            require_top_level = common.split_commas_or_table(option.get("require-top-level")),
            require_semantic_events = option.get("require-semantic-events") or false,
            min_render_packets = tonumber(option.get("min-render-packets") or 0),
        })
    end)
    set_menu {
        usage = "xmake validate-trace-ndjson [options] trace...",
        description = "Validate SMG PC runtime parity NDJSON traces.",
        options = {
            {"-", "require-emulator", "kv", nil, "Require a trace emulator name."},
            {"-", "require-frame", "kv", nil, "Require a frame index."},
            {"-", "require-record-type", "kv", nil, "Require comma-separated record types."},
            {"-", "require-top-level", "kv", nil, "Require comma-separated top-level keys."},
            {"-", "require-semantic-events", "k", nil, "Require at least one semantic_event record."},
            {"-", "min-render-packets", "kv", "0", "Minimum render_packet count."},
            {},
            {nil, "contents", "vs", nil, "Trace files."},
        },
    }

task("aurora-route-smoke")
    on_run(function()
        import("aurora_route_smoke")
        import("core.base.option")
        aurora_route_smoke.run({
            scenarios = option.get("contents") or {},
            list_scenarios = option.get("list-scenarios") or false,
            disc = option.get("disc"),
            pc_bin = option.get("pc-bin"),
            stats_bin = option.get("stats-bin"),
            work_dir = option.get("work-dir"),
            width = option.get("width") and tonumber(option.get("width")) or nil,
            height = option.get("height") and tonumber(option.get("height")) or nil,
            timeout = option.get("timeout") and tonumber(option.get("timeout")) or nil,
            hold_ms = option.get("hold-ms") and tonumber(option.get("hold-ms")) or nil,
            exit_margin_frames = option.get("exit-margin-frames") and tonumber(option.get("exit-margin-frames")) or nil,
            draw_warmup_frames = option.get("draw-warmup-frames") and tonumber(option.get("draw-warmup-frames")) or nil,
            display = option.get("display") and tonumber(option.get("display")) or nil,
            frame_pacing = option.get("frame-pacing") or false,
            reset_save = not option.get("keep-save"),
            build = not option.get("no-build"),
            keep_going = option.get("keep-going") or false,
        })
    end)
    set_menu {
        usage = "xmake aurora-route-smoke [options] [scenario...]",
        description = "Drive and capture Aurora-native title/file-select/picturebook route frames.",
        options = {
            {"-", "list-scenarios", "k", nil, "List available scenarios."},
            {"-", "disc", "kv", nil, "SMG RVZ/WBFS/ISO path."},
            {"-", "pc-bin", "kv", nil, "Path to smg-pc binary."},
            {"-", "stats-bin", "kv", nil, "Path to smg-pc-png-stats binary."},
            {"-", "work-dir", "kv", nil, "Artifact directory."},
            {"-", "width", "kv", "640", "Capture width."},
            {"-", "height", "kv", "480", "Capture height."},
            {"-", "timeout", "kv", "180", "Scenario timeout in seconds."},
            {"-", "hold-ms", "kv", "1000", "Debug hold after trace write."},
            {"-", "exit-margin-frames", "kv", "60", "Frames to allow after screenshot frame."},
            {"-", "draw-warmup-frames", "kv", "2", "Frames to draw before capture."},
            {"-", "display", "kv", nil, "X11 display number for Xvfb."},
            {"-", "frame-pacing", "k", nil, "Run at 60 Hz instead of fast route mode."},
            {"-", "keep-save", "k", nil, "Keep scenario save directories."},
            {"-", "no-build", "k", nil, "Skip building smg-pc and helper tools."},
            {"-", "keep-going", "k", nil, "Continue after scenario failures."},
            {},
            {nil, "contents", "vs", nil, "Scenario names."},
        },
    }

task("render-parity-compare")
    on_run(function()
        import("render_parity")
        import("core.base.option")
        local positional = option.get("contents") or {}
        local function collect_env(mapping)
            local env = {}
            for option_name, env_name in pairs(mapping) do
                local value = option.get(option_name)
                if value ~= nil and value ~= false then
                    env[env_name] = value == true and "1" or value
                end
            end
            return env
        end
        local env = collect_env({
            ["work-dir"] = "SMGPC_PARITY_WORK_DIR",
            ["manifest"] = "SMGPC_PARITY_MANIFEST",
            ["frame"] = "SMGPC_PARITY_FRAME",
            ["pc-frame"] = "SMGPC_PARITY_PC_FRAME",
            ["dolphin-frame"] = "SMGPC_PARITY_DOLPHIN_FRAME",
            ["dolphin-png"] = "SMGPC_PARITY_DOLPHIN_PNG",
            ["pc-png"] = "SMGPC_PARITY_PC_PNG",
            ["timeout-seconds"] = "SMGPC_PARITY_TIMEOUT_SECONDS",
        })
        if option.get("dry-run") then
            env.SMGPC_PARITY_DRY_RUN = "1"
        end
        if option.get("refresh-dolphin") then
            env.SMGPC_PARITY_REFRESH_DOLPHIN = "1"
        end
        if option.get("no-build") then
            env.SMGPC_PARITY_BUILD = "0"
        end
        render_parity.compare({
            scenario = option.get("scenario") or positional[1],
            env = env,
        })
    end)
    set_menu {
        usage = "xmake render-parity-compare [options] [scenario]",
        description = "Capture and compare one Dolphin/pc-port render parity scenario.",
        options = {
            {"-", "scenario", "kv", nil, "Scenario name."},
            {"-", "work-dir", "kv", nil, "Artifact directory."},
            {"-", "manifest", "kv", nil, "Manifest output path."},
            {"-", "frame", "kv", nil, "Override both frame indices."},
            {"-", "pc-frame", "kv", nil, "Override pc-port frame."},
            {"-", "dolphin-frame", "kv", nil, "Override Dolphin frame."},
            {"-", "dolphin-png", "kv", nil, "Use a Dolphin PNG reference."},
            {"-", "pc-png", "kv", nil, "Override pc-port PNG path."},
            {"-", "timeout-seconds", "kv", nil, "Capture timeout in seconds."},
            {"-", "dry-run", "k", nil, "Write scenario configuration only."},
            {"-", "refresh-dolphin", "k", nil, "Refresh Dolphin artifacts."},
            {"-", "no-build", "k", nil, "Skip xmake builds."},
            {},
            {nil, "contents", "vs", nil, "Scenario name."},
        },
    }

task("render-parity-gate")
    on_run(function()
        import("render_parity")
        import("core.base.option")
        local function collect_env(mapping)
            local env = {}
            for option_name, env_name in pairs(mapping) do
                local value = option.get(option_name)
                if value ~= nil and value ~= false then
                    env[env_name] = value == true and "1" or value
                end
            end
            return env
        end
        local env = collect_env({
            ["work-dir"] = "SMGPC_PARITY_GATE_WORK_DIR",
            ["manifest"] = "SMGPC_PARITY_GATE_MANIFEST",
        })
        if option.get("no-build") then
            env.SMGPC_PARITY_BUILD = "0"
        end
        render_parity.gate({
            scenarios = option.get("contents") or {},
            env = env,
        })
    end)
    set_menu {
        usage = "xmake render-parity-gate [options] [scenario...]",
        description = "Run the aggregate render parity gate.",
        options = {
            {"-", "work-dir", "kv", nil, "Gate artifact root."},
            {"-", "manifest", "kv", nil, "Aggregate manifest path."},
            {"-", "no-build", "k", nil, "Skip xmake builds."},
            {},
            {nil, "contents", "vs", nil, "Scenario names."},
        },
    }

task("source-closeness-audit")
    on_run(function()
        import("source_closeness_audit")
        import("core.base.option")
        local argv = {}
        if option.get("output") then
            table.insert(argv, "--output")
            table.insert(argv, option.get("output"))
        end
        if option.get("repo-root") then
            table.insert(argv, "--repo-root")
            table.insert(argv, option.get("repo-root"))
        end
        if option.get("pc-root") then
            table.insert(argv, "--pc-root")
            table.insert(argv, option.get("pc-root"))
        end
        source_closeness_audit.run(argv)
    end)
    set_menu {
        usage = "xmake source-closeness-audit --output DIR [options]",
        description = "Audit pc-port src/Game source closeness and compat inventory.",
        options = {
            {"o", "output", "kv", nil, "Artifact output directory."},
            {"-", "repo-root", "kv", nil, "Repository root containing src/Game and pc-port."},
            {"-", "pc-root", "kv", nil, "pc-port root."},
        },
    }

task("build-wine-shims")
    on_run(function()
        import("wine_shims")
        import("core.base.option")
        local positional = option.get("contents") or {}
        wine_shims.run({
            output_dir = option.get("output-dir") or positional[1],
            cc = option.get("cc"),
        })
    end)
    set_menu {
        usage = "xmake build-wine-shims [options] [output-dir]",
        description = "Build Wine compatibility shim DLLs with llvm-mingw.",
        options = {
            {"o", "output-dir", "kv", nil, "Output directory."},
            {"-", "cc", "kv", nil, "MinGW clang executable."},
            {},
            {nil, "contents", "vs", nil, "Optional output directory."},
        },
    }
