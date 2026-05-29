import("core.base.json")

local common = import("common")
local trace = import("trace_ndjson")

local route_button_script = table.concat({
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
    "5400-5410:A",
}, ";")

local route_pointer_script = table.concat({
    "0-1899:0,0,false",
    "1900-2020:212.935,152.482,true",
    "2021-2079:0,0,false",
    "2080-2200:436,364,true",
    "2201-2299:0,0,false",
    "2300-3699:187,205,true",
    "3700-5200:436,364,true",
    "5201-7000:438,404,true",
    "7001-7899:0,0,false",
}, ";")

local scenarios = {
    title = {
        name = "title",
        frame = 90,
        description = "title screen before scripted A+B input",
        min_nonblack_ratio = 0.003,
        min_render_packets = 1,
        expected_layouts = {"TitleLogo"},
    },
    title_decide = {
        name = "title_decide",
        frame = 420,
        description = "title screen after scripted A+B input has been accepted",
        min_nonblack_ratio = 0.003,
        min_render_packets = 1,
        expected_layouts = {"TitleLogo"},
    },
    file_select = {
        name = "file_select",
        frame = 1900,
        description = "first stable file-select camera after title A+B",
        min_nonblack_ratio = 0.01,
        min_render_packets = 1,
        expected_layouts = {"FileNumber"},
    },
    file_confirm = {
        name = "file_confirm",
        frame = 7000,
        description = "selected-file confirmation path after scripted pointer/A input",
        min_nonblack_ratio = 0.01,
        min_render_packets = 1,
        expected_layouts = {"FileNumber"},
    },
    picturebook = {
        name = "picturebook",
        frame = 7600,
        description = "prologue picturebook first-page route frame",
        min_nonblack_ratio = 0.01,
        min_render_packets = 1,
        expected_layouts = {"PrologueDemo", "IconAButton"},
    },
    picturebook_wait = {
        name = "picturebook_wait",
        frame = 7900,
        description = "prologue picturebook wait frame with A-button prompt",
        min_nonblack_ratio = 0.01,
        min_render_packets = 1,
        expected_layouts = {"PrologueDemo", "IconAButton"},
    },
}

local default_scenarios = {"title", "file_select", "picturebook"}

for _, scenario in pairs(scenarios) do
    scenario.button_script = scenario.button_script or route_button_script
    scenario.pointer_script = scenario.pointer_script or route_pointer_script
end

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

local function list_contains(values, wanted)
    for _, value in ipairs(values) do
        if value == wanted then
            return true
        end
    end
    return false
end

local function parse_image_stats(output)
    local ok, decoded, err = try_call(function()
        return json.decode(output)
    end)
    if not ok then
        raise("smg-pc-png-stats returned invalid JSON: %s", tostring(err))
    end
    return decoded
end

local function image_stats(png_path, stats_bin)
    local output = common.capturev(stats_bin, {png_path})
    return parse_image_stats(output)
end

local function validate_trace_to_log(trace_path, scenario, log_path)
    local summary = trace.validate_trace(trace_path, {
        require_emulator = "pc-port",
        require_frame = scenario.frame,
        require_record_type = {"frame", "render_packet", "semantic_event"},
        require_semantic_events = true,
        min_render_packets = scenario.min_render_packets,
    })
    common.write_file(log_path, trace.format_summaries({summary}))
end

local function terminate_process(proc)
    common.kill_process(proc)
end

local function wait_for_artifacts(proc, trace_path, png_path, app_log, timeout_seconds)
    local deadline = os.mclock() + timeout_seconds * 1000
    while os.mclock() < deadline do
        if common.file_nonempty(trace_path) and common.file_nonempty(png_path) then
            return
        end
        local done = proc:wait(25)
        if done == 1 then
            local missing = {}
            if not common.file_nonempty(trace_path) then
                table.insert(missing, trace_path)
            end
            if not common.file_nonempty(png_path) then
                table.insert(missing, png_path)
            end
            if #missing > 0 then
                raise("smg-pc exited before writing %s; see %s", table.concat(missing, ", "), app_log)
            end
            return
        end
        os.sleep(25)
    end
    raise("timed out waiting for %s and %s; see %s", trace_path, png_path, app_log)
end

local function run_scenario(args, scenario, display, disc_image, pc_bin, stats_bin)
    local scenario_dir = path.join(args.work_dir, scenario.name)
    os.mkdir(scenario_dir)
    local save_dir = path.join(scenario_dir, "save")
    if args.reset_save and os.isdir(save_dir) then
        os.tryrm(save_dir)
    end
    os.mkdir(save_dir)

    local trace_path = path.join(scenario_dir, string.format("%s-frame-%d.trace.ndjson", scenario.name, scenario.frame))
    local png_path = path.join(scenario_dir, string.format("%s-frame-%d.png", scenario.name, scenario.frame))
    local app_log = path.join(scenario_dir, scenario.name .. "-app.log")
    local trace_log = path.join(scenario_dir, scenario.name .. "-trace-validator.log")
    local manifest_path = path.join(scenario_dir, "manifest.json")
    os.tryrm(trace_path)
    os.tryrm(png_path)
    os.tryrm(app_log)
    os.tryrm(trace_log)

    local env = common.make_env({
        DISPLAY = display,
        SMGPC_WINDOW_WIDTH = args.width,
        SMGPC_WINDOW_HEIGHT = args.height,
        SMGPC_ENABLE_VSYNC = "0",
        SMGPC_FRAME_PACING = args.frame_pacing and "1" or "0",
        SMGPC_SAVE_DIR = save_dir,
        SMGPC_PARITY_TRACE_PATH = trace_path,
        SMGPC_PARITY_TRACE_FRAME = scenario.frame,
        SMGPC_SCREENSHOT_PATH = png_path,
        SMGPC_SCREENSHOT_FRAME = scenario.frame,
        SMGPC_EXIT_AFTER_SCREENSHOT = "1",
        SMGPC_EXIT_AFTER_FRAME = scenario.frame + math.max(args.exit_margin_frames, 1),
        SMGPC_SKIP_RENDER_UNTIL_FRAME = math.max(1, scenario.frame - math.max(args.draw_warmup_frames, 0)),
        SMGPC_DEBUG_HOLD_AFTER_TRACE_MS = args.hold_ms,
        SMGPC_DEBUG_WPAD_BUTTON_SCRIPT = scenario.button_script,
        SMGPC_DEBUG_WPAD_POINTER_SCRIPT = scenario.pointer_script,
        SMGPC_SEMANTIC_ANCHOR_CATEGORY = "aurora_route_smoke",
        SMGPC_SEMANTIC_ANCHOR_NAME = scenario.name,
        SMGPC_SEMANTIC_ANCHOR_DETAIL = scenario.description,
    })
    if args.disc == nil then
        env.SMGPC_DISC_IMAGE = disc_image
    end

    local command = {}
    if args.disc ~= nil then
        table.insert(command, "--disc")
        table.insert(command, disc_image)
    end

    print(string.format("aurora-route-smoke: %s frame=%d", scenario.name, scenario.frame))
    local proc, log = common.open_process(pc_bin, command, {
        curdir = common.project_root(),
        env = env,
        log = app_log,
    })
    local ok, _, err = try_call(function()
        wait_for_artifacts(proc, trace_path, png_path, app_log, args.timeout)
    end)
    terminate_process(proc)
    common.close_process(proc, log)
    if not ok then
        raise(tostring(err))
    end

    local stats = image_stats(png_path, stats_bin)
    local expected_heights = {[args.height] = true}
    if args.width == 640 and (args.height == 456 or args.height == 480) then
        expected_heights[456] = true
        expected_heights[480] = true
    end
    if tonumber(stats.width) ~= args.width or not expected_heights[tonumber(stats.height)] then
        local heights = {}
        for height in pairs(expected_heights) do
            table.insert(heights, height)
        end
        table.sort(heights)
        local labels = {}
        for _, height in ipairs(heights) do
            table.insert(labels, string.format("%dx%d", args.width, height))
        end
        raise("%s captured %sx%s, expected %s", png_path, tostring(stats.width), tostring(stats.height), table.concat(labels, " or "))
    end
    if tonumber(stats.nonblack_ratio or 0) < scenario.min_nonblack_ratio then
        raise("%s looks blank: nonblack_ratio=%.5f, required=%.5f", png_path, tonumber(stats.nonblack_ratio or 0), scenario.min_nonblack_ratio)
    end
    if tonumber(stats.max_channel or 0) <= 8 then
        raise("%s has no visible color range", png_path)
    end

    validate_trace_to_log(trace_path, scenario, trace_log)
    local trace_summary = trace.summarize_trace(trace_path)
    local missing_layouts = {}
    for _, layout in ipairs(scenario.expected_layouts) do
        if not list_contains(trace_summary.layout_names or {}, layout) then
            table.insert(missing_layouts, layout)
        end
    end
    if #missing_layouts > 0 then
        raise("%s is missing expected layout packet(s): %s", trace_path, table.concat(missing_layouts, ", "))
    end

    local manifest = {
        scenario = scenario.name,
        description = scenario.description,
        status = "passed",
        frame = scenario.frame,
        expected_layouts = scenario.expected_layouts,
        artifacts = {
            png = png_path,
            trace = trace_path,
            app_log = app_log,
            trace_validator_log = trace_log,
        },
        image_stats = stats,
        trace_summary = trace_summary,
        input = {
            button_script = scenario.button_script,
            pointer_script = scenario.pointer_script,
        },
    }
    common.write_json(manifest_path, manifest)
    print(string.format(
        "aurora-route-smoke: %s passed nonblack=%.4f render_packets=%d png=%s",
        scenario.name,
        tonumber(stats.nonblack_ratio or 0),
        tonumber((trace_summary.record_counts or {}).render_packet or 0),
        png_path))
    return manifest
end

local function disc_image_from_args(args)
    if args.disc and args.disc ~= "" then
        return path.absolute(args.disc)
    end
    local env_disc = os.getenv("SMGPC_DISC_IMAGE")
    if env_disc and env_disc ~= "" then
        return env_disc
    end
    raise("missing disc image: pass --disc <path> or set SMGPC_DISC_IMAGE")
end

local function normalize_args(args)
    args = args or {}
    args.scenarios = args.scenarios or {}
    args.pc_bin = path.absolute(args.pc_bin or path.join(common.project_root(), "build/linux/x86_64/debug/smg-pc"))
    args.stats_bin = path.absolute(args.stats_bin or path.join(common.project_root(), "build/linux/x86_64/debug/smg-pc-png-stats"))
    args.work_dir = path.absolute(args.work_dir or path.join(common.project_root(), ".cache/aurora-route-smoke/latest"))
    args.width = tonumber(args.width or 640)
    args.height = tonumber(args.height or 480)
    args.timeout = tonumber(args.timeout or 180)
    args.hold_ms = tonumber(args.hold_ms or 1000)
    args.exit_margin_frames = tonumber(args.exit_margin_frames or 60)
    args.draw_warmup_frames = tonumber(args.draw_warmup_frames or 2)
    args.display = args.display and tonumber(args.display) or nil
    args.frame_pacing = args.frame_pacing or false
    args.reset_save = args.reset_save ~= false
    args.build = args.build ~= false
    args.keep_going = args.keep_going or false
    return args
end

function run(args)
    args = normalize_args(args)
    if args.list_scenarios then
        local names = {}
        for name in pairs(scenarios) do
            table.insert(names, name)
        end
        table.sort(names)
        for _, name in ipairs(names) do
            local scenario = scenarios[name]
            print(string.format("%s\tframe=%d\t%s", scenario.name, scenario.frame, scenario.description))
        end
        return {status = "listed"}
    end

    local scenario_names = args.scenarios
    if #scenario_names == 0 then
        scenario_names = default_scenarios
    end
    local unknown = {}
    for _, name in ipairs(scenario_names) do
        if scenarios[name] == nil then
            table.insert(unknown, name)
        end
    end
    if #unknown > 0 then
        local names = {}
        for name in pairs(scenarios) do
            table.insert(names, name)
        end
        table.sort(names)
        raise("unknown scenario(s): %s\nknown scenarios: %s", table.concat(unknown, ", "), table.concat(names, " "))
    end

    local disc_image = disc_image_from_args(args)
    if args.build then
        common.runv("xmake", {"build", "smg-pc"}, {curdir = common.project_root()})
        common.runv("xmake", {"build", "smg-pc-png-stats"}, {curdir = common.project_root()})
    end
    if not os.isfile(args.pc_bin) then
        raise("missing smg-pc binary: %s", args.pc_bin)
    end
    if not os.isfile(args.stats_bin) then
        raise("missing smg-pc-png-stats binary: %s", args.stats_bin)
    end

    os.mkdir(args.work_dir)
    local manifests = {}
    local failures = {}
    for _, name in ipairs(scenario_names) do
        local server
        local ok, result, err = try_call(function()
            server = common.start_xvfb(args.width, args.height, args.display, path.join(args.work_dir, name, "xvfb.log"))
            return run_scenario(args, scenarios[name], ":" .. tostring(server.display), disc_image, args.pc_bin, args.stats_bin)
        end)
        common.stop_xvfb(server)
        if ok then
            table.insert(manifests, result)
        else
            local message = tostring(err)
            table.insert(failures, {scenario = name, error = message})
            cprint("${red}aurora-route-smoke: %s failed: %s", name, message)
            if not args.keep_going then
                common.write_json(path.join(args.work_dir, "manifest.json"), {
                    status = "failed",
                    scenarios = manifests,
                    failures = failures,
                })
                raise(message)
            end
        end
    end

    local aggregate = {
        status = (#failures > 0) and "failed" or "passed",
        scenarios = manifests,
        failures = failures,
    }
    local aggregate_path = path.join(args.work_dir, "manifest.json")
    common.write_json(aggregate_path, aggregate)
    print(string.format("aurora-route-smoke: %s manifest=%s", aggregate.status, aggregate_path))
    if #failures > 0 then
        raise("aurora-route-smoke failed")
    end
    return aggregate
end
