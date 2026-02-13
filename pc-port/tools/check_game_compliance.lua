local function normalize_path(value)
    if not value then
        return nil
    end
    return tostring(value):gsub("\\", "/")
end

local function read_text(file_path)
    local file = io.open(file_path, "rb")
    if not file then
        return nil, string.format("unable to read file: %s", file_path)
    end
    local text = file:read("*a")
    file:close()
    return text
end

local function trim_right(value)
    local trimmed, _ = tostring(value):gsub("%s+$", "")
    return trimmed
end

local function split_lines(value)
    local lines = {}
    for line in (tostring(value) .. "\n"):gmatch("(.-)\n") do
        table.insert(lines, line)
    end
    return lines
end

local function escape_pattern(value)
    return tostring(value):gsub("([%%%^%$%(%)%.%[%]%*%+%-%?])", "%%%1")
end

local function strip_integration_blocks(text, begin_marker, end_marker, label)
    local output = {}
    local errors = {}
    local in_block = false
    local line_number = 0

    for line in (text .. "\n"):gmatch("(.-)\n") do
        line_number = line_number + 1
        if line:find(begin_marker, 1, true) then
            if in_block then
                table.insert(errors, string.format("%s:%d nested %s marker", label, line_number, begin_marker))
            end
            in_block = true
        elseif line:find(end_marker, 1, true) then
            if not in_block then
                table.insert(errors, string.format("%s:%d found %s without matching %s", label, line_number, end_marker, begin_marker))
            end
            in_block = false
        elseif not in_block then
            table.insert(output, trim_right(line))
        end
    end

    if in_block then
        table.insert(errors, string.format("%s missing closing %s marker", label, end_marker))
    end

    local normalized = table.concat(output, "\n")
    if normalized ~= "" then
        normalized = normalized .. "\n"
    end
    return normalized, errors
end

local function collect_mirror_files(project_root, mirror_root_relative)
    local mirror_root = path.join(project_root, mirror_root_relative)
    if not os.isdir(mirror_root) then
        return {}
    end

    local files = {}
    local seen = {}
    local function collect(pattern)
        for _, sourcefile in ipairs(os.files(pattern)) do
            local ext = path.extension(sourcefile)
            if ext == ".cpp" or ext == ".hpp" then
                local normalized = normalize_path(path.relative(sourcefile, project_root))
                seen[normalized] = true
            end
        end
    end

    collect(path.join(mirror_root, "**.cpp"))
    collect(path.join(mirror_root, "**.hpp"))

    for filepath in pairs(seen) do
        table.insert(files, filepath)
    end
    table.sort(files)
    return files
end

local function parse_inventory_p0_paths(inventory_path)
    local result = {}
    local file = io.open(inventory_path, "r")
    if not file then
        return result
    end

    for raw in file:lines() do
        if raw:match("^%s*|") then
            local columns = {}
            for column in raw:gmatch("|([^|]+)") do
                local trimmed_column, _ = column:gsub("^%s*(.-)%s*$", "%1")
                table.insert(columns, trimmed_column)
            end

            if #columns >= 2 and columns[1] ~= "Priority" and columns[1] ~= "---" then
                if columns[1] == "P0" and columns[2] ~= "" then
                    result[normalize_path(columns[2])] = true
                end
            end
        end
    end
    file:close()
    return result
end

local function set_from_array(array)
    local values = {}
    for _, value in ipairs(array) do
        values[normalize_path(value)] = true
    end
    return values
end

local function set_difference(left, right)
    local result = {}
    for key in pairs(left) do
        if not right[key] then
            table.insert(result, key)
        end
    end
    table.sort(result)
    return result
end

local function compute_diff_summary(left, right, left_name, right_name, limit)
    local left_lines = split_lines(left)
    local right_lines = split_lines(right)
    local diff_lines = {
        string.format("--- %s", left_name),
        string.format("+++ %s", right_name),
    }
    local i = 1
    local j = 1
    limit = limit or 30

    local function append(line)
        if #diff_lines < limit + 2 then
            table.insert(diff_lines, line)
            return true
        end
        return false
    end

    while i <= #left_lines or j <= #right_lines do
        if #diff_lines >= limit + 2 then
            append("... (diff truncated)")
            break
        end

        local left_line = left_lines[i]
        local right_line = right_lines[j]

        if left_line == right_line then
            i = i + 1
            j = j + 1
        elseif not left_line then
            append(string.format("+ %s", right_line))
            j = j + 1
        elseif not right_line then
            append(string.format("- %s", left_line))
            i = i + 1
        else
            if not append(string.format("- %s", left_line)) then
                break
            end
            if not append(string.format("+ %s", right_line)) then
                break
            end
            i = i + 1
            j = j + 1
        end
    end

    return table.concat(diff_lines, "\n")
end

local function check_render_rewrite_exceptions(scope, project_root)
    local errors = {}
    local expected = set_from_array({ "src/render/", "src/game/layout/" })
    local actual = {}

    if not scope or type(scope) ~= "table" then
        return { "scope must be a table with render_rewrite_exceptions" }
    end

    if type(scope.render_rewrite_exceptions) == "table" then
        actual = set_from_array(scope.render_rewrite_exceptions)
    end

    local missing_or_extra = set_difference(expected, actual)
    for key in pairs(actual) do
        if not expected[key] then
            table.insert(missing_or_extra, key)
        end
    end

    if #missing_or_extra > 0 then
        local actual_sorted = {}
        for key in pairs(actual) do
            table.insert(actual_sorted, key)
        end
        table.sort(actual_sorted)
        table.insert(
            errors,
            string.format(
                "scope.render_rewrite_exceptions must be exactly [\"src/render/\", \"src/game/layout/\"] (found: %s)",
                table.concat(actual_sorted, ", ")
            )
        )
    end

    return errors
end

local function check_compliance(params)
    local errors = {}
    local project_root = normalize_path(params.project_root)
    local repo_root = normalize_path(params.repo_root)

    if not os.isdir(project_root) then
        table.insert(errors, string.format("Project root not found: %s", tostring(params.project_root)))
        return errors
    end

    if not os.isdir(repo_root) then
        table.insert(errors, string.format("Repo root not found: %s", tostring(params.repo_root)))
        return errors
    end

    local policy = params.policy
    if not policy then
        local policy_module = import("tools.game_compliance_policy", {rootdir = project_root})
        if type(policy_module) == "table" and type(policy_module.policy) == "function" then
            policy = policy_module.policy()
        end
    end

    if type(policy) ~= "table" then
        table.insert(errors, "Failed to load policy table")
        return errors
    end

    local markers = policy.markers or {}
    local begin_marker = markers.integration_begin or "SMGPC_INTEGRATION_BEGIN"
    local end_marker = markers.integration_end or "SMGPC_INTEGRATION_END"

    local scope = policy.scope or {}
    local mirror_root = scope.mirror_root
    if type(mirror_root) ~= "string" then
        table.insert(errors, "scope.mirror_root is required")
        return errors
    end

    for _, error_msg in ipairs(check_render_rewrite_exceptions(scope, project_root)) do
        table.insert(errors, error_msg)
    end

    local policy_entries = policy.files
    if type(policy_entries) ~= "table" then
        table.insert(errors, "files must be a list")
        return errors
    end

    local policy_paths = {}
    local policy_by_path = {}
    for _, entry in ipairs(policy_entries) do
        if type(entry) ~= "table" then
            table.insert(errors, "files entries must be mappings")
            goto continue_entry
        end

        local entry_path = entry.path
        if type(entry_path) ~= "string" then
            table.insert(errors, string.format("policy entry missing string path: %s", tostring(entry.path)))
            goto continue_entry
        end

        entry_path = normalize_path(entry_path)
        if policy_by_path[entry_path] then
            table.insert(errors, string.format("duplicate policy entry for %s", entry_path))
            goto continue_entry
        end

        policy_paths[entry_path] = true
        policy_by_path[entry_path] = entry

        ::continue_entry::
    end

    local mirror_files = collect_mirror_files(project_root, mirror_root)
    local mirror_set = {}
    for _, filepath in ipairs(mirror_files) do
        mirror_set[normalize_path(filepath)] = true
        if not policy_paths[normalize_path(filepath)] then
            table.insert(errors, string.format("mirror file is not listed in policy map: %s", filepath))
        end
    end

    for filepath in pairs(policy_paths) do
        if not mirror_set[filepath] then
            table.insert(errors, string.format("policy path does not exist on disk: %s", filepath))
        end
    end

    for local_path, entry in pairs(policy_by_path) do
        local local_file = path.join(project_root, local_path)
        if not os.isfile(local_file) then
            goto continue_file
        end

        local status = entry.status
        local check_divergence = entry.check_divergence == true
        local required_stub_symbols = entry.required_stub_symbols or {}

        if check_divergence then
            local upstream_relative = entry.upstream
            if type(upstream_relative) ~= "string" then
                table.insert(errors, string.format("%s has check_divergence=true but no upstream path", local_path))
                goto continue_file
            end

            local upstream_file = path.join(repo_root, upstream_relative)
            if not os.isfile(upstream_file) then
                table.insert(errors, string.format("%s upstream file missing: %s", local_path, upstream_relative))
                goto continue_file
            end

            local local_text = read_text(local_file)
            local upstream_text = read_text(upstream_file)
            if not local_text then
                table.insert(errors, string.format("unable to read local file: %s", local_path))
                goto continue_file
            end
            if not upstream_text then
                table.insert(errors, string.format("unable to read upstream file: %s", upstream_relative))
                goto continue_file
            end

            local local_stripped, local_strip_errors = strip_integration_blocks(local_text, begin_marker, end_marker, local_path)
            local upstream_stripped, upstream_strip_errors = strip_integration_blocks(upstream_text, begin_marker, end_marker, upstream_relative)
            for _, error_msg in ipairs(local_strip_errors) do
                table.insert(errors, error_msg)
            end
            for _, error_msg in ipairs(upstream_strip_errors) do
                table.insert(errors, error_msg)
            end
            if local_stripped ~= upstream_stripped then
                local diff = compute_diff_summary(local_stripped, upstream_stripped, local_path, upstream_relative, 30)
                table.insert(errors, string.format("%s diverges from %s outside integration markers\n%s", local_path, upstream_relative, diff))
            end
        end

        local has_stub_marker = false
        local local_text = read_text(local_file)
        if not local_text then
            table.insert(errors, string.format("unable to read local file: %s", local_path))
            goto continue_file
        end

        if local_text:find("SMGPC_STUB(", 1, true) then
            has_stub_marker = true
        end

        local requires_stub = status == "ported-stub" or (type(required_stub_symbols) == "table" and #required_stub_symbols > 0)
        if requires_stub then
            if not has_stub_marker then
                table.insert(errors, string.format("%s requires SMGPC_STUB markers but none were found", local_path))
            end
            if type(required_stub_symbols) ~= "table" then
                table.insert(errors, string.format("%s required_stub_symbols must be a list", local_path))
                required_stub_symbols = {}
            end
            for _, symbol in ipairs(required_stub_symbols) do
                if type(symbol) ~= "string" then
                    table.insert(errors, string.format("%s required_stub_symbols contains non-string entry", local_path))
                else
                    local escaped = escape_pattern(symbol)
                    local marker_pattern = "SMGPC_STUB%s*%(%s*" .. escaped .. "%s*%)"
                    if not local_text:find(marker_pattern) then
                        table.insert(errors, string.format("%s missing SMGPC_STUB(%s) marker", local_path, symbol))
                    end
                end
            end
        end

        ::continue_file::
    end

    local decomp_needed = policy.decomp_needed
    if type(decomp_needed) ~= "table" then
        table.insert(errors, "decomp_needed must be a mapping")
        return errors
    end

    local p0_items = decomp_needed.p0
    if type(p0_items) ~= "table" then
        table.insert(errors, "decomp_needed.p0 must be a list")
        p0_items = {}
    end

    local computed_missing = {}
    for _, item in ipairs(p0_items) do
        if type(item) ~= "table" then
            table.insert(errors, string.format("decomp_needed.p0 item is not a mapping: %s", tostring(item)))
        else
            local upstream = item.upstream
            if type(upstream) ~= "string" then
                table.insert(errors, string.format("decomp_needed.p0 item missing upstream: %s", tostring(item)))
            else
                local normalized = normalize_path(upstream)
                if not os.isfile(path.join(repo_root, normalized)) then
                    computed_missing[normalized] = true
                end
            end
        end
    end

    local inventory_file = path.join(project_root, "docs", "decomp-needed-inventory.md")
    local documented_p0 = parse_inventory_p0_paths(inventory_file)
    local only_computed = set_difference(computed_missing, documented_p0)
    local only_documented = set_difference(documented_p0, computed_missing)
    if #only_computed > 0 or #only_documented > 0 then
        table.insert(
            errors,
            string.format(
                "P0 inventory mismatch between computed missing list and docs/decomp-needed-inventory.md (only_computed=%s, only_documented=%s)",
                table.concat(only_computed, ", "),
                table.concat(only_documented, ", ")
            )
        )
    end

    return errors
end

local function check_and_raise(params)
    local errors = check_compliance(params)
    if #errors == 0 then
        return true
    end

    local lines = { "Game compliance check failed:" }
    for index, message in ipairs(errors) do
        table.insert(lines, string.format("%d. %s", index, message))
    end
    raise(table.concat(lines, "\n"))
    return false
end

function check(params)
    return check_and_raise(params)
end

function main(params)
    return check_and_raise(params)
end
