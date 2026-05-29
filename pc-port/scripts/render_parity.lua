import("core.base.json")

local common = import("common")

local function script_join(parts)
    return table.concat(parts, ";")
end

local scenario_defs = {
    custom_frame = function(env)
        local frame = common.env_value("SMGPC_PARITY_FRAME", "1900", env)
        return {
            frame = frame,
            crops = "full=0,0,640,456",
            description = "custom frame capture using SMGPC_PARITY_FRAME",
        }
    end,
    title_wait = function()
        return {
            frame = "1900",
            crop = "150,68,340,292",
            crops = "title_logo=150,68,340,128;press_start=185,270,270,78;sky=0,0,640,456",
            description = "title idle before A+B",
        }
    end,
    title_decide = function()
        return {
            frame = "420",
            pc_frame = "420",
            dolphin_frame = "2000",
            crop = "150,68,340,292",
            crops = "title_logo=150,68,340,128;press_start=185,270,270,78;title_end=0,0,640,456",
            button_script = "160-232:A+B",
            dolphin_button_script = "120-1300:A;1850-1922:A+B",
            description = "title input accepted and title-end transition",
        }
    end,
    file_select_far = function()
        return {
            frame = "1900",
            crop = "0,150,640,240",
            crops = "sky=0,0,640,456;file_items=0,150,640,240;file_number_ui=70,285,500,125;file_info_ui=0,300,640,156",
            button_script = "120-1700:A+B",
            description = "first stable file-select far camera after finite title A+B input",
        }
    end,
    file_confirm_near = function()
        return {
            frame = "5800",
            pc_frame = "7000",
            dolphin_frame = "5800",
            crop = "0,120,640,300",
            crops = "selected_item=160,105,320,260;file_info_ui=0,300,640,156;operation_buttons=35,330,570,110",
            button_script = script_join({
                "120-1700:A+B",
                "1950-1960:A",
                "2130-2140:A",
                "2400-2410:A",
                "2600-2610:A",
                "2800-2810:A",
                "3000-3010:A",
                "3200-3210:A",
                "3400-3410:A",
                "3600-3610:A",
                "3800-3810:A",
                "4000-4010:A",
                "4200-4210:A",
                "4400-4410:A",
                "4600-4610:A",
                "4800-4810:A",
                "5000-5010:A",
            }),
            pointer_script = script_join({
                "0-1899:0,0,false",
                "1900-2020:212.935,152.482,true",
                "2021-2079:0,0,false",
                "2080-2200:436,364,true",
                "2201-2299:0,0,false",
                "2300-3699:187,205,true",
                "3700-5200:436,364,true",
                "5201-5799:0,0,false",
            }),
            dolphin_button_script = "120-1700:A+B;2100-2120:A;2700-2720:A;3150-3170:A;4600-4620:A;5500-5520:A",
            dolphin_pointer_script = "0-1799:0,0,false;1800-2200:271,264,true;2201-2299:0,0,false;2300-2800:386,385,true;2801-2999:0,0,false;3000-3250:187,251,true;3251-4449:0,0,false;4450-4700:386,385,true;4701-5299:0,0,false;5300-5600:271,264,true;5601-5799:0,0,false",
            description = "selected-file near camera/file-confirm state after create/reselect",
        }
    end,
    picturebook_page1 = function()
        local button = "120-1700:A+B;1950-1960:A;2130-2140:A;2400-2410:A;2600-2610:A;2800-2810:A;3000-3010:A;3200-3210:A;3400-3410:A;3600-3610:A;3800-3810:A;4000-4010:A;4200-4210:A;4400-4410:A;4600-4610:A;4800-4810:A;5000-5010:A;5400-5410:A"
        local pointer = "0-1899:0,0,false;1900-2020:212.935,152.482,true;2021-2079:0,0,false;2080-2200:436,364,true;2201-2299:0,0,false;2300-3699:187,205,true;3700-5200:436,364,true;5201-7000:438,404,true;7001-7599:0,0,false"
        return {
            frame = "7600",
            crop = "70,35,500,360",
            crops = "picturebook_page=70,35,500,360;prologue_text=75,330,490,90;a_button=500,330,90,90",
            button_script = button,
            pointer_script = pointer,
            dolphin_button_script = "120-1700:A+B;2100-2120:A;2700-2720:A;3150-3170:A;4600-4620:A;5500-5520:A;6800-6840:A",
            dolphin_pointer_script = "0-1799:0,0,false;1800-2200:271,264,true;2201-2299:0,0,false;2300-2800:386,385,true;2801-2999:0,0,false;3000-3250:187,251,true;3251-4449:0,0,false;4450-4700:386,385,true;4701-5299:0,0,false;5300-5600:271,264,true;5601-5999:0,0,false;6000-7000:430,420,true;7001-7599:0,0,false",
            description = "prologue picturebook first page after file create/reselect/start",
        }
    end,
    picturebook_wait = function()
        local button = "120-1700:A+B;1950-1960:A;2130-2140:A;2400-2410:A;2600-2610:A;2800-2810:A;3000-3010:A;3200-3210:A;3400-3410:A;3600-3610:A;3800-3810:A;4000-4010:A;4200-4210:A;4400-4410:A;4600-4610:A;4800-4810:A;5000-5010:A;5400-5410:A"
        return {
            frame = "7900",
            crop = "70,35,500,360",
            crops = "picturebook_page=70,35,500,360;prologue_text=75,330,490,90;a_button=500,330,90,90",
            button_script = button,
            pointer_script = "0-1899:0,0,false;1900-2020:212.935,152.482,true;2021-2079:0,0,false;2080-2200:436,364,true;2201-2299:0,0,false;2300-3699:187,205,true;3700-5200:436,364,true;5201-7000:438,404,true;7001-7899:0,0,false",
            dolphin_button_script = "120-1700:A+B;2100-2120:A;2700-2720:A;3150-3170:A;4600-4620:A;5500-5520:A;6800-6840:A",
            dolphin_pointer_script = "0-1799:0,0,false;1800-2200:271,264,true;2201-2299:0,0,false;2300-2800:386,385,true;2801-2999:0,0,false;3000-3250:187,251,true;3251-4449:0,0,false;4450-4700:386,385,true;4701-5299:0,0,false;5300-5600:271,264,true;5601-5999:0,0,false;6000-7000:430,420,true;7001-7899:0,0,false",
            description = "prologue picturebook page wait with A icon after file create/reselect/start",
        }
    end,
}

local default_gate_scenarios = {
    "title_wait",
    "title_decide",
    "file_select_far",
    "file_confirm_near",
    "picturebook_page1",
    "picturebook_wait",
}

local function try_call(fn)
    local result
    local caught
    local ok = try {
        function()
            result = fn()
            return true
        end,
        catch {
            function(errors)
                caught = tostring(errors)
                return false
            end,
        },
    }
    return ok, result, caught
end

local function parse_crop_definitions(crops)
    local out = {}
    for entry in tostring(crops or ""):gmatch("[^;]+") do
        local name, rect = entry:match("^([^=]+)=(.+)$")
        if name and rect then
            table.insert(out, {name = name, rect = rect})
        end
    end
    if #out == 0 then
        table.insert(out, {name = "full", rect = "0,0,640,456"})
    end
    return out
end

local function validate_artifact_pair(label, png_path, trace_path)
    local has_png = common.file_nonempty(png_path)
    local has_trace = common.file_nonempty(trace_path)
    if has_png ~= has_trace then
        raise("%s artifact pair is incomplete: png=%s trace=%s\n  png: %s\n  trace: %s",
            label,
            common.file_state(png_path),
            common.file_state(trace_path),
            png_path,
            trace_path)
    end
end

local function scenario_config(name, env)
    local make = scenario_defs[name]
    if not make then
        local known = {}
        for scenario_name in pairs(scenario_defs) do
            table.insert(known, scenario_name)
        end
        table.sort(known)
        raise("unknown SMGPC parity scenario: %s\nknown scenarios: %s", name, table.concat(known, " "))
    end
    local config = make(env)
    config.scenario = name
    return config
end

local function resolve_compare(args)
    local env = common.make_env(args and args.env or {})
    local pc_root = common.project_root()
    local repo_root = common.repo_root()
    local scenario = common.env_value("SMGPC_PARITY_SCENARIO", args and args.scenario or nil, env) or "custom_frame"
    local config = scenario_config(scenario, env)
    local pc_frame = common.env_value("SMGPC_PARITY_PC_FRAME", common.env_value("SMGPC_PARITY_FRAME", config.pc_frame or config.frame, env), env)
    local dolphin_frame = common.env_value("SMGPC_PARITY_DOLPHIN_FRAME", common.env_value("SMGPC_PARITY_FRAME", config.dolphin_frame or config.frame, env), env)
    local build_mode = common.env_value("SMGPC_PARITY_XMAKE_MODE", "debug", env)
    local default_work_dir = scenario == "custom_frame"
        and path.join(pc_root, ".cache/render-parity")
        or path.join(pc_root, ".cache/render-parity", scenario)
    local work_dir = common.env_value("SMGPC_PARITY_WORK_DIR", default_work_dir, env)
    local default_cached_dolphin_png = path.join(pc_root, ".cache/dolphin-reference-dump/Frames/framedump_" .. tostring(dolphin_frame) .. ".png")

    local dolphin_png_is_user_supplied = false
    local dolphin_png_is_cached_reference = false
    local dolphin_png
    if common.env_value("SMGPC_PARITY_DOLPHIN_PNG", nil, env) then
        dolphin_png = common.env_value("SMGPC_PARITY_DOLPHIN_PNG", nil, env)
        dolphin_png_is_user_supplied = true
    elseif common.file_nonempty(default_cached_dolphin_png) then
        dolphin_png = default_cached_dolphin_png
        dolphin_png_is_cached_reference = true
    else
        dolphin_png = path.join(work_dir, "dolphin-frame-" .. tostring(dolphin_frame) .. ".png")
    end

    local pc_save_dir_is_default = false
    local pc_save_dir = common.env_value("SMGPC_PARITY_PC_SAVE_DIR", nil, env)
    if not pc_save_dir then
        pc_save_dir = path.join(work_dir, "pc-save")
        pc_save_dir_is_default = true
    end

    local ctx = {
        env = env,
        pc_root = pc_root,
        repo_root = repo_root,
        scenario = scenario,
        scenario_description = config.description,
        scenario_frame = config.frame,
        frame = pc_frame,
        pc_frame = pc_frame,
        dolphin_frame = dolphin_frame,
        build_mode = build_mode,
        work_dir = work_dir,
        dolphin_trace = common.env_value("SMGPC_PARITY_DOLPHIN_TRACE", path.join(work_dir, "dolphin-frame-" .. tostring(dolphin_frame) .. ".trace.sqlite"), env),
        dolphin_png = dolphin_png,
        dolphin_png_is_user_supplied = dolphin_png_is_user_supplied,
        dolphin_png_is_cached_reference = dolphin_png_is_cached_reference,
        pc_png = common.env_value("SMGPC_PARITY_PC_PNG", path.join(work_dir, "pcport-frame-" .. tostring(pc_frame) .. ".png"), env),
        pc_trace = common.env_value("SMGPC_PARITY_PC_TRACE", path.join(work_dir, "pcport-frame-" .. tostring(pc_frame) .. ".trace.sqlite"), env),
        dolphin_log = path.join(work_dir, "dolphin-frame-" .. tostring(dolphin_frame) .. ".log"),
        pc_log = path.join(work_dir, "pcport-frame-" .. tostring(pc_frame) .. ".log"),
        diff_log = path.join(work_dir, "visual-diff-frame-" .. tostring(pc_frame) .. ".log"),
        trace_sqlite = common.env_value("SMGPC_PARITY_TRACE_SQLITE", path.join(work_dir, "traces.sqlite"), env),
        trace_pack_log = path.join(work_dir, "trace-pack-frame-" .. tostring(pc_frame) .. ".log"),
        trace_compare_log = path.join(work_dir, "trace-compare-frame-" .. tostring(pc_frame) .. ".log"),
        manifest_path = common.env_value("SMGPC_PARITY_MANIFEST", path.join(work_dir, "manifest.json"), env),
        dolphin_user = common.env_value("SMGPC_PARITY_DOLPHIN_USER", path.join(work_dir, "dolphin-user"), env),
        dolphin_shm = common.env_value("SMGPC_DOLPHIN_SHM_DIR", path.join(work_dir, "dolphin-shm"), env),
        pc_save_dir = pc_save_dir,
        reset_pc_save = common.env_value("SMGPC_PARITY_RESET_PC_SAVE", pc_save_dir_is_default and "1" or "0", env),
        dolphin_bin = common.env_value("SMGPC_DOLPHIN_BIN", path.join(pc_root, "dolphin/build-nogui-libcxx/Binaries/dolphin-emu-nogui"), env),
        game_image = common.env_value("SMGPC_DOLPHIN_GAME", path.join(repo_root, "Super Mario Wii - Galaxy Adventure (Korea).rvz"), env),
        pc_bin = common.env_value("SMGPC_PC_BIN", path.join(pc_root, "build/linux/x86_64/" .. build_mode .. "/smg-pc"), env),
        visual_diff_bin = common.env_value("SMGPC_VISUAL_DIFF_BIN", path.join(pc_root, "build/linux/x86_64/" .. build_mode .. "/smg-pc-visual-diff"), env),
        trace_pack_bin = common.env_value("SMGPC_TRACE_PACK_BIN", path.join(pc_root, "build/linux/x86_64/" .. build_mode .. "/smg-pc-trace-pack-sqlite"), env),
        trace_compare_bin = common.env_value("SMGPC_TRACE_COMPARE_BIN", path.join(pc_root, "build/linux/x86_64/" .. build_mode .. "/smg-pc-trace-compare-sqlite"), env),
        dolphin_platform = common.env_value("SMGPC_DOLPHIN_PLATFORM", "x11", env),
        dolphin_video_backend = common.env_value("SMGPC_DOLPHIN_VIDEO_BACKEND", "Software", env),
        timeout_seconds = tonumber(common.env_value("SMGPC_PARITY_TIMEOUT_SECONDS", "240", env)),
        max_full_rms = common.env_value("SMGPC_PARITY_MAX_FULL_NORMALIZED_RMS", "0.35", env),
        max_crop_rms = common.env_value("SMGPC_PARITY_MAX_CROP_NORMALIZED_RMS", "", env),
        crop = common.env_value("SMGPC_PARITY_CROP", config.crop or "", env),
        crops = config.crops,
        semantic_category = common.env_value("SMGPC_PARITY_SEMANTIC_CATEGORY", "capture", env),
        semantic_name = common.env_value("SMGPC_PARITY_SEMANTIC_NAME", scenario, env),
        semantic_detail = common.env_value("SMGPC_PARITY_SEMANTIC_DETAIL", "render_parity_compare scenario " .. scenario .. " pc frame " .. tostring(pc_frame) .. " dolphin frame " .. tostring(dolphin_frame), env),
        pc_button_script = common.env_value("SMGPC_PARITY_PC_WPAD_BUTTON_SCRIPT", config.button_script or "", env),
        pc_pointer_script = common.env_value("SMGPC_PARITY_PC_WPAD_POINTER_SCRIPT", config.pointer_script or "", env),
        dolphin_button_script = common.env_value("SMGPC_PARITY_DOLPHIN_WPAD_BUTTON_SCRIPT", config.dolphin_button_script or config.button_script or "", env),
        dolphin_pointer_script = common.env_value("SMGPC_PARITY_DOLPHIN_WPAD_POINTER_SCRIPT", config.dolphin_pointer_script or config.pointer_script or "", env),
        dolphin_reference_status = "pending",
    }
    return ctx
end

local function write_manifest(ctx, status, detail)
    local manifest = {
        scenario = ctx.scenario,
        scenario_description = ctx.scenario_description,
        status = status,
        detail = detail or "",
        frame = tonumber(ctx.frame),
        pc_frame = tonumber(ctx.pc_frame),
        dolphin_frame = tonumber(ctx.dolphin_frame),
        build_mode = ctx.build_mode,
        command = "SMGPC_PARITY_SCENARIO=" .. ctx.scenario .. " xmake render-parity-compare",
        env = {
            build = common.env_value("SMGPC_PARITY_BUILD", "1", ctx.env),
            refresh_dolphin = common.env_value("SMGPC_PARITY_REFRESH_DOLPHIN", "0", ctx.env),
            timeout_seconds = tostring(ctx.timeout_seconds),
            aurora_backend = "x11",
            dolphin_platform = ctx.dolphin_platform,
            dolphin_video_backend = ctx.dolphin_video_backend,
            work_dir = ctx.work_dir,
            pc_save_dir = ctx.pc_save_dir,
            reset_pc_save = ctx.reset_pc_save,
        },
        tools = {
            dolphin_bin = ctx.dolphin_bin,
            game_image = ctx.game_image,
            pc_bin = ctx.pc_bin,
            visual_diff_bin = ctx.visual_diff_bin,
            trace_pack_bin = ctx.trace_pack_bin,
            trace_compare_bin = ctx.trace_compare_bin,
        },
        dolphin_reference_status = ctx.dolphin_reference_status,
        thresholds = {
            max_full_normalized_rms = ctx.max_full_rms,
            max_crop_normalized_rms = ctx.max_crop_rms,
        },
        primary_crop = ctx.crop,
        crop_definitions = parse_crop_definitions(ctx.crops),
        semantic_anchor = {
            category = ctx.semantic_category,
            name = ctx.semantic_name,
            detail = ctx.semantic_detail,
        },
        dolphin_input = {
            button_script = ctx.dolphin_button_script,
            pointer_script = ctx.dolphin_pointer_script,
        },
        pc_input = {
            button_script = ctx.pc_button_script,
            pointer_script = ctx.pc_pointer_script,
        },
        artifacts = {
            dolphin_png = {path = ctx.dolphin_png, state = common.file_state(ctx.dolphin_png), bytes = common.file_size(ctx.dolphin_png)},
            dolphin_trace = {path = ctx.dolphin_trace, state = common.file_state(ctx.dolphin_trace), bytes = common.file_size(ctx.dolphin_trace)},
            pc_png = {path = ctx.pc_png, state = common.file_state(ctx.pc_png), bytes = common.file_size(ctx.pc_png)},
            pc_trace = {path = ctx.pc_trace, state = common.file_state(ctx.pc_trace), bytes = common.file_size(ctx.pc_trace)},
            trace_sqlite = {path = ctx.trace_sqlite, state = common.file_state(ctx.trace_sqlite), bytes = common.file_size(ctx.trace_sqlite)},
            logs = {
                dolphin = ctx.dolphin_log,
                pc = ctx.pc_log,
                visual_diff = ctx.diff_log,
                trace_pack = ctx.trace_pack_log,
                trace_compare = ctx.trace_compare_log,
            },
        },
    }
    common.write_json(ctx.manifest_path, manifest)
end

local function build_targets(ctx)
    if common.env_value("SMGPC_PARITY_BUILD", "1", ctx.env) ~= "1" then
        return
    end
    local env_extra = {
        CC = common.env_value("CC", "clang-22", ctx.env),
        CXX = common.env_value("CXX", "clang++-22", ctx.env),
    }
    common.runv("xmake", {"f", "-m", ctx.build_mode}, {curdir = ctx.pc_root, env_extra = env_extra})
    for _, target in ipairs({
        "smg-pc",
        "smg-pc-visual-diff",
        "smg-pc-trace-pack-sqlite",
        "smg-pc-trace-compare-sqlite",
        "smg-pc-trace-inspect-sqlite",
    }) do
        common.runv("xmake", {"build", target}, {curdir = ctx.pc_root, env_extra = env_extra})
    end
end

local function wait_for_pair(proc, png_path, trace_path, timeout_seconds, log_path)
    local deadline = os.mclock() + timeout_seconds * 1000
    while os.mclock() < deadline do
        if common.file_nonempty(png_path) and common.file_nonempty(trace_path) then
            return true
        end
        local done = proc:wait(1000)
        if done == 1 then
            return common.file_nonempty(png_path) and common.file_nonempty(trace_path)
        end
    end
    if log_path then
        cprint("${yellow}%s", common.tail(log_path, 80))
    end
    return false
end

local function run_dolphin_capture(ctx)
    if common.env_value("SMGPC_PARITY_REFRESH_DOLPHIN", "0", ctx.env) ~= "1" and common.file_nonempty(ctx.dolphin_png) and common.file_nonempty(ctx.dolphin_trace) then
        if ctx.dolphin_png_is_user_supplied then
            ctx.dolphin_reference_status = "user-supplied"
        elseif ctx.dolphin_png_is_cached_reference then
            ctx.dolphin_reference_status = "cached"
        else
            ctx.dolphin_reference_status = "cached-work-dir"
        end
        return
    end

    if common.env_value("SMGPC_PARITY_REFRESH_DOLPHIN", "0", ctx.env) ~= "1" then
        validate_artifact_pair("Dolphin cached/reference", ctx.dolphin_png, ctx.dolphin_trace)
    end
    if not os.isexec(ctx.dolphin_bin) then
        raise("missing Dolphin NoGUI binary: %s\nbuild it with pc-port/dolphin/build-nogui-libcxx or set SMGPC_DOLPHIN_BIN", ctx.dolphin_bin)
    end
    if not common.read_binary_contains(ctx.dolphin_bin, "SMGPC_DOLPHIN_TRACE_PATH") then
        raise("Dolphin binary does not appear to contain the SMGPC Dolphin trace hooks required for SQLite parity traces: %s", ctx.dolphin_bin)
    end
    if not os.isfile(ctx.game_image) then
        raise("missing Dolphin game image: %s\nset SMGPC_DOLPHIN_GAME to the Korean SMG RVZ/ISO path", ctx.game_image)
    end

    if not ctx.dolphin_png_is_cached_reference and (not ctx.dolphin_png_is_user_supplied or common.env_value("SMGPC_PARITY_REFRESH_DOLPHIN", "0", ctx.env) == "1" or not common.file_nonempty(ctx.dolphin_png)) then
        os.tryrm(ctx.dolphin_png)
    end
    os.tryrm(ctx.dolphin_trace)
    ctx.dolphin_reference_status = "refreshed"

    local server = common.start_xvfb(640, 480, nil, path.join(ctx.work_dir, "dolphin-xvfb.log"))
    local env = common.make_env({
        DISPLAY = ":" .. tostring(server.display),
        SMGPC_DOLPHIN_SHM_DIR = ctx.dolphin_shm,
        SMGPC_DOLPHIN_CAPTURE_FRAME = ctx.dolphin_frame,
        SMGPC_DOLPHIN_CAPTURE_PATH = ctx.dolphin_png,
        SMGPC_DOLPHIN_TRACE_FRAME = ctx.dolphin_frame,
        SMGPC_DOLPHIN_TRACE_PATH = ctx.dolphin_trace,
        SMGPC_DOLPHIN_TRACE_WINDOW = common.env_value("SMGPC_DOLPHIN_TRACE_WINDOW", "0", ctx.env),
        SMGPC_DOLPHIN_WPAD_BUTTON_SCRIPT = ctx.dolphin_button_script,
        SMGPC_DOLPHIN_WPAD_POINTER_SCRIPT = ctx.dolphin_pointer_script,
        SMGPC_DOLPHIN_SEMANTIC_ANCHOR_CATEGORY = ctx.semantic_category,
        SMGPC_DOLPHIN_SEMANTIC_ANCHOR_NAME = ctx.semantic_name,
        SMGPC_DOLPHIN_SEMANTIC_ANCHOR_DETAIL = ctx.semantic_detail,
    })
    local proc, log
    local ok, _, err = try_call(function()
        proc, log = common.open_process(ctx.dolphin_bin, {
            "-u", ctx.dolphin_user,
            "-p", ctx.dolphin_platform,
            "-v", ctx.dolphin_video_backend,
            "-C", "Graphics.Settings.FrameDumpsResolutionType=2",
            "-C", "Wiimote.Wiimote1.Source=1",
            "-C", "Wiimote.Wiimote2.Source=0",
            "-C", "Wiimote.Wiimote3.Source=0",
            "-C", "Wiimote.Wiimote4.Source=0",
            "-e", ctx.game_image,
        }, {env = env, log = ctx.dolphin_log})
        if not wait_for_pair(proc, ctx.dolphin_png, ctx.dolphin_trace, ctx.timeout_seconds, ctx.dolphin_log) then
            raise("Dolphin did not write %s and %s within %ss", ctx.dolphin_png, ctx.dolphin_trace, tostring(ctx.timeout_seconds))
        end
        common.kill_process(proc)
        validate_artifact_pair("Dolphin captured", ctx.dolphin_png, ctx.dolphin_trace)
    end)
    common.kill_process(proc)
    common.close_process(proc, log)
    common.stop_xvfb(server)
    if not ok then
        raise(tostring(err))
    end
end

local function run_pc_capture(ctx)
    os.tryrm(ctx.pc_png)
    os.tryrm(ctx.pc_trace)

    local env_overlay = {
        SMGPC_ENABLE_VSYNC = common.env_value("SMGPC_ENABLE_VSYNC", "0", ctx.env),
        SMGPC_EVENT_POLL_INTERVAL = common.env_value("SMGPC_EVENT_POLL_INTERVAL", "8", ctx.env),
        SMGPC_ASYNC_SCREENSHOT_PNG = common.env_value("SMGPC_ASYNC_SCREENSHOT_PNG", "1", ctx.env),
        SMGPC_SAVE_DIR = ctx.pc_save_dir,
        SMGPC_SKIP_RENDER_UNTIL_FRAME = common.env_value("SMGPC_SKIP_RENDER_UNTIL_FRAME", ctx.pc_frame, ctx.env),
        SMGPC_WINDOW_WIDTH = "640",
        SMGPC_WINDOW_HEIGHT = "456",
        SMGPC_SCREENSHOT_PATH = ctx.pc_png,
        SMGPC_SCREENSHOT_FRAME = ctx.pc_frame,
        SMGPC_PARITY_TRACE_PATH = ctx.pc_trace,
        SMGPC_PARITY_TRACE_FRAME = ctx.pc_frame,
        SMGPC_SEMANTIC_ANCHOR_CATEGORY = ctx.semantic_category,
        SMGPC_SEMANTIC_ANCHOR_NAME = ctx.semantic_name,
        SMGPC_SEMANTIC_ANCHOR_DETAIL = ctx.semantic_detail,
        SMGPC_EXIT_AFTER_SCREENSHOT = "1",
    }
    if ctx.pc_button_script ~= "" then
        env_overlay.SMGPC_DEBUG_WPAD_BUTTON_SCRIPT = ctx.pc_button_script
    end
    if ctx.pc_pointer_script ~= "" then
        env_overlay.SMGPC_DEBUG_WPAD_POINTER_SCRIPT = ctx.pc_pointer_script
    end

    local server = common.start_xvfb(640, 456, nil, path.join(ctx.work_dir, "pc-xvfb.log"))
    env_overlay.DISPLAY = ":" .. tostring(server.display)
    local proc, log
    local ok, _, err = try_call(function()
        proc, log = common.open_process(ctx.pc_bin, {}, {
            curdir = ctx.pc_root,
            env = common.make_env(env_overlay),
            log = ctx.pc_log,
        })
        if not wait_for_pair(proc, ctx.pc_png, ctx.pc_trace, ctx.timeout_seconds, ctx.pc_log) then
            raise("pc-port did not write %s and %s within %ss", ctx.pc_png, ctx.pc_trace, tostring(ctx.timeout_seconds))
        end
        common.kill_process(proc)
        validate_artifact_pair("pc-port captured", ctx.pc_png, ctx.pc_trace)
    end)
    common.kill_process(proc)
    common.close_process(proc, log)
    common.stop_xvfb(server)
    if not ok then
        cprint("${yellow}%s", common.tail(ctx.pc_log, 80))
        raise(tostring(err))
    end
end

function compare(args)
    local ctx = resolve_compare(args or {})
    os.mkdir(ctx.work_dir, ctx.dolphin_user, ctx.dolphin_shm)
    if ctx.reset_pc_save == "1" then
        os.tryrm(ctx.pc_save_dir)
    end
    os.mkdir(ctx.pc_save_dir)

    if common.env_value("SMGPC_PARITY_DRY_RUN", "0", ctx.env) == "1" then
        ctx.dolphin_reference_status = "dry-run"
        write_manifest(ctx, "dry-run", "scenario configuration written without build or capture")
        print("scenario=" .. ctx.scenario)
        print("frame=" .. tostring(ctx.frame))
        print("pc_frame=" .. tostring(ctx.pc_frame))
        print("dolphin_frame=" .. tostring(ctx.dolphin_frame))
        print("manifest=" .. ctx.manifest_path)
        return ctx
    end

    local ok, _, err = try_call(function()
        if ctx.build_mode ~= "debug" then
            raise("render-parity-compare requires SMGPC_PARITY_XMAKE_MODE=debug because it builds debug-only trace/visual tools and uses debug-only runtime capture hooks")
        end
        build_targets(ctx)
        run_dolphin_capture(ctx)
        run_pc_capture(ctx)

        os.tryrm(ctx.trace_sqlite)
        local pack_ok = common.runv(ctx.trace_pack_bin, {"--output", ctx.trace_sqlite, ctx.dolphin_trace, ctx.pc_trace}, {
            stdout = ctx.trace_pack_log,
            stderr = ctx.trace_pack_log,
            check = false,
        })
        if not pack_ok then
            cprint("${yellow}%s", common.tail(ctx.trace_pack_log, 80))
            raise("trace pack failed for scenario %s", ctx.scenario)
        end
        local compare_ok = common.runv(ctx.trace_compare_bin, {"--database", ctx.trace_sqlite}, {
            stdout = ctx.trace_compare_log,
            stderr = ctx.trace_compare_log,
            check = false,
        })
        if not compare_ok then
            cprint("${yellow}%s", common.tail(ctx.trace_compare_log, 80))
            raise("trace compare failed for scenario %s", ctx.scenario)
        end

        local diff_args = {}
        if ctx.crop ~= "" then
            table.insert(diff_args, "--crop")
            table.insert(diff_args, ctx.crop)
        end
        if ctx.max_full_rms ~= "" then
            table.insert(diff_args, "--max-full-normalized-rms")
            table.insert(diff_args, ctx.max_full_rms)
        end
        if ctx.max_crop_rms ~= "" then
            table.insert(diff_args, "--max-crop-normalized-rms")
            table.insert(diff_args, ctx.max_crop_rms)
        end
        table.insert(diff_args, ctx.dolphin_png)
        table.insert(diff_args, ctx.pc_png)
        local diff_ok = common.runv(ctx.visual_diff_bin, diff_args, {
            stdout = ctx.diff_log,
            stderr = ctx.diff_log,
            check = false,
        })
        io.write(common.read_file(ctx.diff_log) or "")
        if not diff_ok then
            raise("visual diff failed for scenario %s", ctx.scenario)
        end
        write_manifest(ctx, "passed", "captured Dolphin/PC SQLite traces, packed the analysis database, compared traces, and ran visual diff")
    end)
    if not ok then
        write_manifest(ctx, "failed", tostring(err))
        raise(tostring(err))
    end
    return ctx
end

function gate(args)
    args = args or {}
    local env = common.make_env(args.env or {})
    local scenarios = args.scenarios
    if not scenarios or #scenarios == 0 then
        local env_scenarios = common.env_value("SMGPC_PARITY_GATE_SCENARIOS", nil, env)
        scenarios = env_scenarios and common.split_words(env_scenarios) or default_gate_scenarios
    end

    local work_root = common.env_value("SMGPC_PARITY_GATE_WORK_DIR", path.join(common.project_root(), ".cache/render-parity-gate/latest"), env)
    local aggregate_manifest = common.env_value("SMGPC_PARITY_GATE_MANIFEST", path.join(work_root, "manifest.json"), env)
    os.mkdir(work_root)

    local gate_build = common.env_value("SMGPC_PARITY_BUILD", "1", env)
    local entries = {}
    local overall = common.env_value("SMGPC_PARITY_DRY_RUN", "0", env) == "1" and "dry-run" or "passed"
    for index, scenario in ipairs(scenarios) do
        local scenario_work_dir = path.join(work_root, scenario)
        os.mkdir(scenario_work_dir)
        local scenario_build = gate_build
        if index ~= 1 and gate_build == "1" then
            scenario_build = "0"
        end
        print("render-parity-gate: " .. scenario)
        compare({
            scenario = scenario,
            env = {
                SMGPC_PARITY_BUILD = scenario_build,
                SMGPC_PARITY_WORK_DIR = scenario_work_dir,
            },
        })
        local manifest_path = path.join(scenario_work_dir, "manifest.json")
        local manifest = common.load_json(manifest_path)
        local status = manifest.status or "missing"
        if status ~= "passed" and not (overall == "dry-run" and status == "dry-run") then
            overall = "failed"
        end
        table.insert(entries, {
            scenario = manifest.scenario or scenario,
            status = status,
            pc_frame = manifest.pc_frame or manifest.frame,
            dolphin_frame = manifest.dolphin_frame or manifest.frame,
            manifest = manifest_path,
            visual_diff = (((manifest.artifacts or {}).logs or {}).visual_diff or ""),
            trace_compare = (((manifest.artifacts or {}).logs or {}).trace_compare or ""),
        })
    end

    common.write_json(aggregate_manifest, {
        status = overall,
        scenario_count = #entries,
        scenarios = entries,
    })
    print(string.format("render-parity-gate: %s %s", overall, aggregate_manifest))
    if overall == "failed" then
        raise("render parity gate failed")
    end
    return {
        status = overall,
        manifest = aggregate_manifest,
        scenarios = entries,
    }
end
