#!/usr/bin/env luajit

local function trim(value)
    local text = tostring(value or ""):gsub("^%s+", "")
    return (text:gsub("%s+$", ""))
end

local function join(...)
    local out = ""
    for _, part in ipairs({ ... }) do
        if part ~= nil and tostring(part) ~= "" then
            part = tostring(part)
            if out == "" then
                out = part
            elseif out:sub(-1) == "/" then
                out = out .. part:gsub("^/+", "")
            else
                out = out .. "/" .. part:gsub("^/+", "")
            end
        end
    end
    return out
end

local function shell_quote(value)
    value = tostring(value)
    return "'" .. value:gsub("'", [['"'"']]) .. "'"
end

local function capture(command)
    local pipe = io.popen(command .. " 2>&1")
    if not pipe then
        return nil, false
    end
    local output = pipe:read("*a") or ""
    local ok = pipe:close()
    return trim(output), ok == true or ok == 0
end

local function run(command)
    local output, ok = capture(command)
    if not ok then
        io.stderr:write("source_closeness_audit.lua: command failed: " .. command .. "\n" .. tostring(output) .. "\n")
        os.exit(1)
    end
    return output
end

local function file_exists(path)
    local file = io.open(path, "rb")
    if file then
        file:close()
        return true
    end
    return false
end

local function dir_exists(path)
    local ok = os.execute("test -d " .. shell_quote(path))
    return ok == true or ok == 0
end

local function mkdir_p(path)
    local ok = os.execute("mkdir -p " .. shell_quote(path))
    if not (ok == true or ok == 0) then
        io.stderr:write("source_closeness_audit.lua: could not create " .. path .. "\n")
        os.exit(1)
    end
end

local function read_binary(path)
    local file = io.open(path, "rb")
    if not file then
        return nil
    end
    local text = file:read("*a")
    file:close()
    return text
end

local function write_file(path, text)
    local file = io.open(path, "w")
    if not file then
        io.stderr:write("source_closeness_audit.lua: could not write " .. path .. "\n")
        os.exit(1)
    end
    file:write(text)
    file:close()
end

local function split_lines(text)
    local lines = {}
    for line in tostring(text or ""):gmatch("[^\r\n]+") do
        table.insert(lines, line)
    end
    return lines
end

local function list_files(root)
    if not dir_exists(root) then
        return {}
    end
    local output = run("find " .. shell_quote(root) .. " -type f \\( -name '*.cpp' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' \\) | sort")
    return split_lines(output)
end

local function relative(path, root)
    if path == root then
        return ""
    end
    local prefix = root
    if prefix:sub(-1) ~= "/" then
        prefix = prefix .. "/"
    end
    if path:sub(1, #prefix) == prefix then
        return path:sub(#prefix + 1)
    end
    return path
end

local function extension(path)
    return path:match("(%.[^./]+)$") or ""
end

local function dirname(path)
    return path:match("^(.*)/[^/]*$") or "."
end

local function basename(path)
    return path:match("([^/]+)$") or path
end

local function find_case_variant(path)
    if file_exists(path) then
        return path, nil
    end

    local dir = dirname(path)
    if not dir_exists(dir) then
        return path, nil
    end

    local output, ok = capture("find " .. shell_quote(dir) .. " -maxdepth 1 -type f -iname " .. shell_quote(basename(path)) .. " | sort")
    if not ok or output == "" then
        return path, nil
    end

    local wanted = path:lower()
    for _, line in ipairs(split_lines(output)) do
        if line:lower() == wanted then
            return line, path
        end
    end

    return path, nil
end

local function normalize_line_endings(text)
    text = tostring(text or ""):gsub("\r\n", "\n"):gsub("\r", "\n")
    if text ~= "" and text:sub(-1) ~= "\n" then
        text = text .. "\n"
    end
    return text
end

local function normalize_trailing_ws(text)
    local lines = {}
    for line in (normalize_line_endings(text)):gmatch("([^\n]*)\n") do
        table.insert(lines, (line:gsub("[ \t]+$", "")))
    end
    return table.concat(lines, "\n") .. "\n"
end

local function is_preprocessor_if(line)
    return line:match("^%s*#%s*if") ~= nil
end

local function is_preprocessor_endif(line)
    return line:match("^%s*#%s*endif") ~= nil
end

local function is_debug_guard_start(line)
    return line:match("^%s*#%s*ifndef%s+NDEBUG") or line:match("^%s*#%s*if%s+!%s*defined%s*%(%s*NDEBUG%s*%)")
end

local function strip_debug_blocks(text)
    local out = {}
    local depth = 0
    local keep_release_else = false

    for line in (normalize_line_endings(text)):gmatch("([^\n]*)\n") do
        if depth == 0 and is_debug_guard_start(line) then
            depth = 1
            keep_release_else = false
        elseif depth > 0 then
            if keep_release_else and depth > 1 then
                table.insert(out, line)
            elseif keep_release_else and depth == 1 and not is_preprocessor_endif(line) then
                table.insert(out, line)
            end

            if is_preprocessor_if(line) then
                depth = depth + 1
            elseif line:match("^%s*#%s*else") and depth == 1 then
                keep_release_else = true
            elseif is_preprocessor_endif(line) then
                depth = depth - 1
                if depth == 0 then
                    keep_release_else = false
                end
            end
        else
            table.insert(out, line)
        end
    end

    return table.concat(out, "\n") .. "\n"
end

local function count_pattern(text, pattern)
    local count = 0
    for _ in tostring(text or ""):gmatch(pattern) do
        count = count + 1
    end
    return count
end

local function normalize_compile_only_adaptations(text)
    text = normalize_trailing_ws(text)
    text = text:gsub("%[%[maybe_unused%]%]%s*", "")
    text = text:gsub("const%s+JMapInfoIter&%s+[%a_][%w_]*%s*%)", "const JMapInfoIter&)")
    return text
end

local release_observer_patterns = {
    ["Map/FileSelectItem.hpp"] = {
        "wasPointed%(",
        "wasPointingCleared%(",
        "didTurnToFront%(",
        "getTurnToFrontFrameCount%(",
        "getPosition%(",
    },
    ["Map/FileSelectItem.cpp"] = {
        "FileSelectItem::wasPointed%(",
        "FileSelectItem::wasPointingCleared%(",
        "FileSelectItem::didTurnToFront%(",
        "FileSelectItem::getTurnToFrontFrameCount%(",
        "FileSelectItem::getPosition%(",
    },
    ["Screen/FileSelectInfo.hpp"] = {
        "getFileNumber%(",
        "getStarNum%(",
        "getStarPieceNum%(",
        "isSelectedMario%(",
    },
    ["Screen/FileSelectInfo.cpp"] = {
        "FileSelectInfo::getFileNumber%(",
        "FileSelectInfo::getStarNum%(",
        "FileSelectInfo::getStarPieceNum%(",
        "FileSelectInfo::isSelectedMario%(",
    },
}

local function count_release_observer_candidates(rel, stripped_release_text)
    local count = count_pattern(stripped_release_text, "[dD]ebug[A-Z]") + count_pattern(stripped_release_text, "[oO]bserver")
    for _, pattern in ipairs(release_observer_patterns[rel] or {}) do
        count = count + count_pattern(stripped_release_text, pattern)
    end
    return count
end

local function sha256(path)
    if not file_exists(path) then
        return ""
    end
    local output = run("sha256sum " .. shell_quote(path))
    return output:match("^(%x+)") or ""
end

local function tsv(value)
    value = tostring(value or "")
    value = value:gsub("[\t\r\n]+", " ")
    return trim(value)
end

local function parse_args(argv)
    local args = {}
    local i = 1
    while i <= #argv do
        local key = argv[i]
        if key:sub(1, 2) == "--" then
            key = key:sub(3)
            local next_value = argv[i + 1]
            if next_value and next_value:sub(1, 2) ~= "--" then
                args[key] = next_value
                i = i + 1
            else
                args[key] = true
            end
        else
            args[#args + 1] = argv[i]
        end
        i = i + 1
    end
    return args
end

local function realpath(path)
    local output, ok = capture("readlink -f -- " .. shell_quote(path))
    if ok and output ~= "" then
        return output
    end
    return path
end

local function detect_roots(args)
    local cwd = run("pwd")
    local repo_root = args["repo-root"]
    local pc_root = args["pc-root"]

    if repo_root then
        repo_root = realpath(repo_root)
    elseif dir_exists(join(cwd, "pc-port", "src", "Game")) and dir_exists(join(cwd, "src", "Game")) then
        repo_root = realpath(cwd)
    elseif dir_exists(join(cwd, "src", "Game")) and dir_exists(join(cwd, "..", "src", "Game")) then
        repo_root = realpath(join(cwd, ".."))
    else
        io.stderr:write("source_closeness_audit.lua: could not detect repo root; pass --repo-root\n")
        os.exit(1)
    end

    if pc_root then
        pc_root = realpath(pc_root)
    else
        pc_root = join(repo_root, "pc-port")
    end

    return repo_root, pc_root
end

local target_prefixes = {
    "Camera/",
    "Demo/Prologue",
    "LiveActor/",
    "Map/FileSelect",
    "Map/FileSelector",
    "NPC/MiiFacePartsHolder",
    "NameObj/",
    "Scene/",
    "Screen/BackButton",
    "Screen/BrosButton",
    "Screen/ButtonPaneController",
    "Screen/CaptureScreenDirector",
    "Screen/EncouragePal60Window",
    "Screen/FileSelect",
    "Screen/IconAButton",
    "Screen/InformationMessage",
    "Screen/LayoutActor",
    "Screen/LayoutManager",
    "Screen/LayoutPaneCtrl",
    "Screen/Manual2P",
    "Screen/MiiConfirmIcon",
    "Screen/MiiSelect",
    "Screen/PictureBook",
    "Screen/Prologue",
    "Screen/ReplaceTagProcessor",
    "Screen/SaveIcon",
    "Screen/ScreenAlphaCapture",
    "Screen/SimpleLayout",
    "Screen/SysInfoWindow",
    "Screen/TitleSequenceProduct",
    "Screen/YesNoController",
    "System/",
    "Util/",
}

local function is_target_surface(rel)
    for _, prefix in ipairs(target_prefixes) do
        if rel:sub(1, #prefix) == prefix then
            return true
        end
    end
    return false
end

local function root_candidate(repo_root, rel)
    local ext = extension(rel)
    local path = nil
    if ext == ".cpp" or ext == ".c" then
        path = join(repo_root, "src", "Game", rel)
    else
        path = join(repo_root, "include", "Game", rel)
    end
    return find_case_variant(path)
end

local function declaration_candidate(repo_root, rel)
    local ext = extension(rel)
    if ext ~= ".cpp" and ext ~= ".c" then
        return nil
    end

    local stem = rel:gsub("%.[^./]+$", "")
    for _, header_ext in ipairs({ ".hpp", ".h" }) do
        local path = join(repo_root, "include", "Game", stem .. header_ext)
        local candidate = find_case_variant(path)
        if file_exists(candidate) then
            return candidate
        end
    end

    return nil
end

local function classify(pc_text, root_text)
    if not root_text then
        return "decomp-needed", "no root src/Game or include/Game counterpart"
    end
    if pc_text == root_text then
        return "exact-source", "byte-for-byte match"
    end

    local pc_trim = normalize_trailing_ws(pc_text)
    local root_trim = normalize_trailing_ws(root_text)
    if pc_trim == root_trim then
        return "compile-only", "line ending or trailing whitespace only"
    end

    if normalize_compile_only_adaptations(pc_trim) == normalize_compile_only_adaptations(root_trim) then
        return "compile-only", "warning-only adaptation such as unused parameter/name annotation"
    end

    if strip_debug_blocks(pc_trim) == strip_debug_blocks(root_trim) then
        return "debug-only", "diff disappears after stripping NDEBUG debug blocks"
    end

    return "compat-temporary", "source exists but pc-port behavior/shape differs"
end

local function compat_group(rel)
    if rel:match("^scene/") or rel:match("^runtime/Runtime") or rel:match("^runtime/Scene") or rel:match("^runtime/NameObj") then
        return "scene-sequence"
    end
    if rel:match("^render/Effect") then
        return "effects"
    end
    if rel:match("^render/LiveActorModel") then
        return "actor-model"
    end
    if rel:match("^render/") or rel:match("^camera/") or rel:match("^layout/Lyt") or rel:match("^layout/Brlyt") or rel:match("^layout/Brlan") or
        rel:match("^runtime/Jut") then
        return "render-gx-j3d-brlyt"
    end
    if rel:match("^resource/") or rel:match("^layout/Brfnt") then
        return "resource-message-font-texture"
    end
    if rel:match("^runtime/RVLFace") then
        return "rfl-mii"
    end
    if rel:match("^runtime/ParityTrace") then
        return "trace-proof"
    end
    return "platform-compat"
end

local function is_promoted_render_compat_file(rel)
    return rel:match("^render/EffectResourceCompat%.") or
        rel:match("^render/GX") or
        rel:match("^render/J3d") or
        rel:match("^render/JMathTrig%.") or
        rel:match("^render/LightDataCompat%.") or
        rel:match("^render/LiveActorModelCompat%.")
end

local function compat_contract(group)
    local contracts = {
        ["scene-sequence"] = "native-shaped sequence, scene, placement, scheduler, and story route services",
        ["render-gx-j3d-brlyt"] = "GX/J3D/BRLYT/camera/light/render-state services expected by original Game code",
        ["resource-message-font-texture"] = "DVD/RARC/Yaz0/BCSV/BMG/BRFNT/TPL/text resource services",
        ["rfl-mii"] = "RVLFaceLib and Mii data/model/icon services",
        ["effects"] = "effect resource lookup and actor-bound effect services",
        ["trace-proof"] = "debug-only parity trace and proof instrumentation",
        ["actor-model"] = "host model backing for LiveActor/PartsModel without changing actor logic",
        ["platform-compat"] = "miscellaneous host platform contract for original Game code",
    }
    return contracts[group] or contracts["platform-compat"]
end

local function add_count(counts, key)
    counts[key] = (counts[key] or 0) + 1
end

local function ordered_keys(counts)
    local keys = {}
    for key in pairs(counts) do
        table.insert(keys, key)
    end
    table.sort(keys)
    return keys
end

local function run_audit(argv, options)
local emit = (options and options.print) or print
local args = parse_args(argv or {})
if args.help or args.h then
    emit([[
usage:
  luajit scripts/source_closeness_audit.lua --output DIR [--repo-root DIR] [--pc-root DIR]

Outputs:
  source-closeness.tsv
  source-closeness-summary.md
  required-migration.tsv
  release-boundary.tsv
  decomp-declarations.tsv
  compile-only-allowlist.tsv
  compat-inventory.tsv
]])
    return { help = true }
end

local repo_root, pc_root = detect_roots(args)
local output_dir = args.output or args.o
if not output_dir then
    io.stderr:write("source_closeness_audit.lua: missing --output DIR\n")
    os.exit(1)
end
mkdir_p(output_dir)

local pc_game_root = join(pc_root, "src", "Game")
local compat_roots = {
    { root = join(pc_root, "src", "camera"), prefix = "camera" },
    { root = join(pc_root, "src", "layout"), prefix = "layout" },
    { root = join(pc_root, "src", "render"), prefix = "render", include = is_promoted_render_compat_file },
    { root = join(pc_root, "src", "resource"), prefix = "resource" },
    { root = join(pc_root, "src", "runtime"), prefix = "runtime" },
    { root = join(pc_root, "src", "scene"), prefix = "scene" },
}
local rows = {}
local counts = {}
local target_counts = {}
local release_rows = {}

for _, pc_path in ipairs(list_files(pc_game_root)) do
    local rel = relative(pc_path, pc_game_root)
    local source_path, normalized_from = root_candidate(repo_root, rel)
    local declaration_path = declaration_candidate(repo_root, rel)
    local pc_text = read_binary(pc_path) or ""
    local root_text = file_exists(source_path) and read_binary(source_path) or nil
    local classification, note = classify(pc_text, root_text)
    if normalized_from then
        note = "root path case-normalized from " .. relative(normalized_from, repo_root) .. "; " .. note
    end
    if classification == "decomp-needed" and declaration_path then
        note = "no root implementation counterpart; declaration counterpart " .. relative(declaration_path, repo_root)
    end
    local target = is_target_surface(rel)
    local stripped = strip_debug_blocks(normalize_trailing_ws(pc_text))
    local row = {
        rel = rel,
        pc_path = relative(pc_path, repo_root),
        source_path = file_exists(source_path) and relative(source_path, repo_root) or "",
        declaration_path = declaration_path and relative(declaration_path, repo_root) or "",
        classification = classification,
        target = target and "yes" or "no",
        pc_sha256 = sha256(pc_path),
        source_sha256 = file_exists(source_path) and sha256(source_path) or "",
        debug_guard_count = tostring(count_pattern(pc_text, "#%s*ifndef%s+NDEBUG") + count_pattern(pc_text, "#%s*if%s+!%s*defined%s*%(%s*NDEBUG%s*%)")),
        unguarded_smgpc_count = tostring(count_pattern(stripped, "SMGPC_")),
        unguarded_observer_api_count = tostring(count_release_observer_candidates(rel, stripped)),
        note = note,
    }
    table.insert(rows, row)
    add_count(counts, classification)
    if target then
        add_count(target_counts, classification)
    end
    if row.unguarded_smgpc_count ~= "0" or row.unguarded_observer_api_count ~= "0" or row.debug_guard_count ~= "0" then
        table.insert(release_rows, row)
    end
end

table.sort(rows, function(a, b)
    return a.rel < b.rel
end)

local compat_rows = {}
local compat_counts = {}
for _, spec in ipairs(compat_roots) do
    for _, compat_path in ipairs(list_files(spec.root)) do
        local rel = spec.prefix .. "/" .. relative(compat_path, spec.root)
        if spec.include == nil or spec.include(rel) then
            local group = compat_group(rel)
            table.insert(compat_rows, {
                rel = rel,
                path = relative(compat_path, repo_root),
                group = group,
                contract = compat_contract(group),
                sha256 = sha256(compat_path),
            })
            add_count(compat_counts, group)
        end
    end
end
table.sort(compat_rows, function(a, b)
    return a.rel < b.rel
end)

local function write_source_tsv(path)
    local lines = {
        table.concat({
            "rel_path",
            "pc_path",
            "source_path",
            "classification",
            "target_surface",
            "pc_sha256",
            "source_sha256",
            "debug_guard_count",
            "unguarded_smgpc_count",
            "unguarded_observer_api_count",
            "note",
        }, "\t"),
    }
    for _, row in ipairs(rows) do
        table.insert(lines, table.concat({
            tsv(row.rel),
            tsv(row.pc_path),
            tsv(row.source_path),
            tsv(row.classification),
            tsv(row.target),
            tsv(row.pc_sha256),
            tsv(row.source_sha256),
            tsv(row.debug_guard_count),
            tsv(row.unguarded_smgpc_count),
            tsv(row.unguarded_observer_api_count),
            tsv(row.note),
        }, "\t"))
    end
    write_file(path, table.concat(lines, "\n") .. "\n")
end

local function write_required_migration(path)
    local lines = {
        table.concat({ "rel_path", "classification", "target_surface", "source_path", "note" }, "\t"),
    }
    for _, row in ipairs(rows) do
        if row.classification == "compat-temporary" or row.classification == "decomp-needed" then
            table.insert(lines, table.concat({
                tsv(row.rel),
                tsv(row.classification),
                tsv(row.target),
                tsv(row.source_path),
                tsv(row.note),
            }, "\t"))
        end
    end
    write_file(path, table.concat(lines, "\n") .. "\n")
end

local function write_release_boundary(path)
    local lines = {
        table.concat({
            "rel_path",
            "classification",
            "debug_guard_count",
            "unguarded_smgpc_count",
            "unguarded_observer_api_count",
            "note",
        }, "\t"),
    }
    for _, row in ipairs(release_rows) do
        table.insert(lines, table.concat({
            tsv(row.rel),
            tsv(row.classification),
            tsv(row.debug_guard_count),
            tsv(row.unguarded_smgpc_count),
            tsv(row.unguarded_observer_api_count),
            tsv("counts are measured after stripping NDEBUG-only blocks for release-facing columns"),
        }, "\t"))
    end
    write_file(path, table.concat(lines, "\n") .. "\n")
end

local function write_compat_inventory(path)
    local lines = {
        table.concat({ "rel_path", "pc_path", "group", "owned_contract", "sha256" }, "\t"),
    }
    for _, row in ipairs(compat_rows) do
        table.insert(lines, table.concat({
            tsv(row.rel),
            tsv(row.path),
            tsv(row.group),
            tsv(row.contract),
            tsv(row.sha256),
        }, "\t"))
    end
    write_file(path, table.concat(lines, "\n") .. "\n")
end

local function write_decomp_declarations(path)
    local lines = {
        table.concat({ "rel_path", "classification", "target_surface", "declaration_path", "note" }, "\t"),
    }
    for _, row in ipairs(rows) do
        if row.classification == "decomp-needed" then
            table.insert(lines, table.concat({
                tsv(row.rel),
                tsv(row.classification),
                tsv(row.target),
                tsv(row.declaration_path),
                tsv(row.note),
            }, "\t"))
        end
    end
    write_file(path, table.concat(lines, "\n") .. "\n")
end

local function compile_allowances_for(row)
    local allowances = {}
    local text = read_binary(join(repo_root, row.pc_path)) or ""
    local line_no = 0
    for line in (normalize_line_endings(text)):gmatch("([^\n]*)\n") do
        line_no = line_no + 1
        if line:find("const JMapInfoIter&)", 1, true) then
            table.insert(allowances, {
                rel = row.rel,
                pc_path = row.pc_path,
                source_path = row.source_path,
                pc_line = tostring(line_no),
                adaptation = "unnamed-unused-parameter",
                reason = "compile-only unused parameter warning cleanup; source body is otherwise normalized to root",
                removal_trigger = "remove when the copied source builds with the original named unused JMapInfoIter parameter",
            })
        end
        if line:find("[[maybe_unused]]", 1, true) then
            table.insert(allowances, {
                rel = row.rel,
                pc_path = row.pc_path,
                source_path = row.source_path,
                pc_line = tostring(line_no),
                adaptation = "maybe-unused-local",
                reason = "compile-only unused local warning cleanup; the source expression is preserved",
                removal_trigger = "remove when the copied source builds with the original local declaration",
            })
        end
    end
    return allowances
end

local function write_compile_only_allowlist(path)
    local lines = {
        table.concat({ "rel_path", "pc_path", "source_path", "pc_line", "adaptation", "reason", "removal_trigger" }, "\t"),
    }
    for _, row in ipairs(rows) do
        if row.classification == "compile-only" then
            for _, allowance in ipairs(compile_allowances_for(row)) do
                table.insert(lines, table.concat({
                    tsv(allowance.rel),
                    tsv(allowance.pc_path),
                    tsv(allowance.source_path),
                    tsv(allowance.pc_line),
                    tsv(allowance.adaptation),
                    tsv(allowance.reason),
                    tsv(allowance.removal_trigger),
                }, "\t"))
            end
        end
    end
    write_file(path, table.concat(lines, "\n") .. "\n")
end

local function count_rows(predicate)
    local count = 0
    for _, row in ipairs(rows) do
        if predicate(row) then
            count = count + 1
        end
    end
    return count
end

local function write_summary(path)
    local lines = {}
    table.insert(lines, "# Source-Closeness Audit")
    table.insert(lines, "")
    table.insert(lines, "Generated: " .. os.date("!%Y-%m-%dT%H:%M:%SZ"))
    table.insert(lines, "Repo root: `" .. repo_root .. "`")
    table.insert(lines, "PC root: `" .. pc_root .. "`")
    table.insert(lines, "")
    table.insert(lines, "## Compatibility-Layer Boundary")
    table.insert(lines, "")
    table.insert(lines, "Promoted support directories (`pc-port/src/camera`, `pc-port/src/layout`, `pc-port/src/resource`, `pc-port/src/runtime`, `pc-port/src/scene`, and selected compatibility files in `pc-port/src/render`) are inventoried as custom compatibility-layer code and are not counted as failed original-source parity. The audited original game-code surface is `pc-port/src/Game`.")
    table.insert(lines, "")
    table.insert(lines, "## Classification Policy")
    table.insert(lines, "")
    table.insert(lines, "- `exact-source`: byte-for-byte match with root `src/Game` or `include/Game`.")
    table.insert(lines, "- `compile-only`: only line endings, trailing whitespace, or warning-only unused parameter/local annotations differ.")
    table.insert(lines, "- `debug-only`: the diff disappears after guarded `NDEBUG` debug blocks are stripped.")
    table.insert(lines, "- `compat-temporary`: root source exists, but the PC file has non-debug behavioral or API-shape differences.")
    table.insert(lines, "- `decomp-needed`: no root source/header counterpart exists at the expected path.")
    table.insert(lines, "")
    table.insert(lines, "This is intentionally conservative. Anything not proven exact, compile-only, or debug-only is migration work.")
    table.insert(lines, "")
    table.insert(lines, "## Counts")
    table.insert(lines, "")
    table.insert(lines, "| Classification | All audited Game files | Target surface files |")
    table.insert(lines, "| --- | ---: | ---: |")
    for _, key in ipairs({ "exact-source", "compile-only", "debug-only", "compat-temporary", "decomp-needed" }) do
        table.insert(lines, string.format("| `%s` | %d | %d |", key, counts[key] or 0, target_counts[key] or 0))
    end
    table.insert(lines, "")
    table.insert(lines, string.format("Audited original game-code files: %d", #rows))
    table.insert(lines, string.format("Target surface files: %d", count_rows(function(row) return row.target == "yes" end)))
    table.insert(lines, string.format("Compatibility-layer files inventoried separately: %d", #compat_rows))
    table.insert(lines, string.format("Decomp-needed files with root declaration counterparts: %d", count_rows(function(row) return row.classification == "decomp-needed" and row.declaration_path ~= "" end)))
    table.insert(lines, string.format("Compile-only files requiring allowlist entries: %d", count_rows(function(row) return row.classification == "compile-only" end)))
    table.insert(lines, "")
    table.insert(lines, "## Release Boundary")
    table.insert(lines, "")
    table.insert(lines, string.format("Files with guarded debug probes or release-facing observer candidates: %d", #release_rows))
    table.insert(lines, "")
    for _, row in ipairs(release_rows) do
        if row.unguarded_smgpc_count ~= "0" or row.unguarded_observer_api_count ~= "0" then
            table.insert(lines, string.format("- `%s`: unguarded_smgpc=%s, unguarded_observer_candidates=%s", row.rel, row.unguarded_smgpc_count, row.unguarded_observer_api_count))
        end
    end
    table.insert(lines, "")
    table.insert(lines, "## Required Migration")
    table.insert(lines, "")
    table.insert(lines, string.format("Files requiring migration or decomp work: %d", count_rows(function(row) return row.classification == "compat-temporary" or row.classification == "decomp-needed" end)))
    table.insert(lines, "")
    for _, row in ipairs(rows) do
        if row.target == "yes" and (row.classification == "compat-temporary" or row.classification == "decomp-needed") then
            table.insert(lines, "- `" .. row.rel .. "`: `" .. row.classification .. "`")
        end
    end
    table.insert(lines, "")
    table.insert(lines, "## Compatibility Inventory")
    table.insert(lines, "")
    table.insert(lines, "| Group | Files |")
    table.insert(lines, "| --- | ---: |")
    for _, key in ipairs(ordered_keys(compat_counts)) do
        table.insert(lines, string.format("| `%s` | %d |", key, compat_counts[key]))
    end
    table.insert(lines, "")
    table.insert(lines, "Detailed artifacts: `source-closeness.tsv`, `required-migration.tsv`, `release-boundary.tsv`, `decomp-declarations.tsv`, `compile-only-allowlist.tsv`, and `compat-inventory.tsv`.")
    table.insert(lines, "")
    write_file(path, table.concat(lines, "\n"))
end

write_source_tsv(join(output_dir, "source-closeness.tsv"))
write_required_migration(join(output_dir, "required-migration.tsv"))
write_release_boundary(join(output_dir, "release-boundary.tsv"))
write_decomp_declarations(join(output_dir, "decomp-declarations.tsv"))
write_compile_only_allowlist(join(output_dir, "compile-only-allowlist.tsv"))
write_compat_inventory(join(output_dir, "compat-inventory.tsv"))
write_summary(join(output_dir, "source-closeness-summary.md"))

emit("source closeness audit written to " .. output_dir)
emit("audited_original_game_files=" .. tostring(#rows))
emit("compat_layer_files=" .. tostring(#compat_rows))
for _, key in ipairs({ "exact-source", "compile-only", "debug-only", "compat-temporary", "decomp-needed" }) do
    emit(key .. "=" .. tostring(counts[key] or 0))
end

return {
    output_dir = output_dir,
    repo_root = repo_root,
    pc_root = pc_root,
    audited_original_game_files = #rows,
    compat_layer_files = #compat_rows,
    counts = counts,
    target_counts = target_counts,
    release_boundary_files = #release_rows,
    decomp_declaration_files = count_rows(function(row)
        return row.classification == "decomp-needed" and row.declaration_path ~= ""
    end),
    compile_only_files = count_rows(function(row)
        return row.classification == "compile-only"
    end),
}
end

local M = {
    run = run_audit,
}

local first_arg = select(1, ...)
if first_arg == "scripts.source_closeness_audit" or first_arg == "source_closeness_audit" then
    return M
end

run_audit({ ... })

return M
