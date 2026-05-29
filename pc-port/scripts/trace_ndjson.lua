import("core.base.json")

local common = import("common")

TRACE_SCHEMA = "smgpc-trace-ndjson-v1"

local function is_integer(value)
    return type(value) == "number" and value == math.floor(value)
end

local function fail(pathname, line_no, message)
    local prefix = pathname
    if line_no then
        prefix = prefix .. ":" .. tostring(line_no)
    end
    raise("%s: %s", prefix, message)
end

local function require_condition(condition, pathname, line_no, message)
    if not condition then
        fail(pathname, line_no, message)
    end
end

local function line_iter(text)
    text = tostring(text or "")
    if text ~= "" and text:sub(-1) ~= "\n" then
        text = text .. "\n"
    end
    return text:gmatch("([^\r\n]*)\r?\n")
end

local function decode_json(pathname, line_no, text)
    local record
    local decode_error
    local ok = try {
        function()
            record = json.decode(text)
            return true
        end,
        catch {
            function(errors)
                decode_error = tostring(errors)
                return false
            end,
        },
    }
    if not ok then
        fail(pathname, line_no, "invalid JSON: " .. tostring(decode_error))
    end
    return record
end

function validate_trace(pathname, args)
    args = args or {}
    local counts = {}
    local frame_index
    local emulator
    local top_level_keys = {}
    local line_count = 0
    local text = common.read_file(pathname)
    if text == nil then
        fail(pathname, nil, "could not read trace")
    end

    local line_no = 0
    for line in line_iter(text) do
        line_no = line_no + 1
        local stripped = line:gsub("^%s+", ""):gsub("%s+$", "")
        if stripped ~= "" then
            line_count = line_count + 1
            local record = decode_json(pathname, line_no, stripped)

            require_condition(record.schema == TRACE_SCHEMA, pathname, line_no, "unexpected trace schema")

            local record_type = record.record_type
            require_condition(type(record_type) == "string" and record_type ~= "", pathname, line_no, "missing record_type")
            counts[record_type] = (counts[record_type] or 0) + 1

            local record_emulator = record.emulator
            require_condition(type(record_emulator) == "string" and record_emulator ~= "", pathname, line_no, "missing emulator")
            if emulator == nil then
                emulator = record_emulator
            end
            require_condition(record_emulator == emulator, pathname, line_no, "emulator changed within trace")

            local record_frame = record.frame_index
            require_condition(is_integer(record_frame), pathname, line_no, "missing integer frame_index")
            if frame_index == nil then
                frame_index = record_frame
            end
            require_condition(record_frame == frame_index, pathname, line_no, "frame_index changed within trace")

            local payload = record.payload
            require_condition(record.payload ~= nil, pathname, line_no, "missing payload")

            if record_type == "trace_meta" then
                require_condition(type(payload) == "table", pathname, line_no, "trace_meta payload must be an object")
                require_condition(payload.source_schema == "smgpc-runtime-parity-trace-v1", pathname, line_no, "unexpected source_schema")
                require_condition(payload.requested_frame == frame_index, pathname, line_no, "trace_meta requested_frame mismatch")
            elseif record_type == "frame" then
                require_condition(type(payload) == "table", pathname, line_no, "frame payload must be an object")
                require_condition(payload.index == frame_index, pathname, line_no, "frame payload index mismatch")
                require_condition(type(payload.framebuffer) == "table", pathname, line_no, "frame payload missing framebuffer")
            elseif record_type == "top_level" then
                local key = record.key
                require_condition(type(key) == "string" and key ~= "", pathname, line_no, "top_level record missing key")
                top_level_keys[key] = true
            elseif record_type == "render_packet" then
                require_condition(type(payload) == "table", pathname, line_no, "render_packet payload must be an object")
                if payload.frame_index ~= nil then
                    require_condition(is_integer(payload.frame_index), pathname, line_no, "render_packet payload frame_index must be an integer")
                end
                require_condition(type(payload.render_pass) == "string", pathname, line_no, "render_packet missing render_pass")
            elseif record_type == "semantic_event" then
                require_condition(type(payload) == "table", pathname, line_no, "semantic_event payload must be an object")
                require_condition(type(payload.category) == "string", pathname, line_no, "semantic_event missing category")
                require_condition(type(payload.name) == "string", pathname, line_no, "semantic_event missing name")
            end
        end
    end

    require_condition(line_count > 0, pathname, nil, "empty trace")
    require_condition(frame_index ~= nil, pathname, nil, "missing trace frame")
    require_condition(emulator ~= nil, pathname, nil, "missing trace emulator")

    if args.require_emulator ~= nil then
        require_condition(emulator == args.require_emulator, pathname, nil, "trace emulator does not match requirement")
    end
    if args.require_frame ~= nil then
        require_condition(frame_index == tonumber(args.require_frame), pathname, nil, "trace frame does not match requirement")
    end
    for _, record_type in ipairs(args.require_record_type or {}) do
        require_condition((counts[record_type] or 0) > 0, pathname, nil, "missing required record_type " .. record_type)
    end
    for _, key in ipairs(args.require_top_level or {}) do
        require_condition(top_level_keys[key], pathname, nil, "missing required top_level key " .. key)
    end
    require_condition((counts.render_packet or 0) >= tonumber(args.min_render_packets or 0), pathname, nil, "render_packet count below minimum")
    if args.require_semantic_events then
        require_condition((counts.semantic_event or 0) > 0, pathname, nil, "missing semantic_event records")
    end

    return {
        path = pathname,
        status = "passed",
        frame_index = frame_index,
        emulator = emulator,
        lines = line_count,
        render_packet = counts.render_packet or 0,
        semantic_event = counts.semantic_event or 0,
        top_level = counts.top_level or 0,
    }
end

function summarize_trace(pathname)
    local counts = {}
    local layout_names = {}
    local semantic_names = {}
    local frame_index
    local text = common.read_file(pathname) or ""
    for line in line_iter(text) do
        local stripped = line:gsub("^%s+", ""):gsub("%s+$", "")
        if stripped ~= "" then
            local record = decode_json(pathname, nil, stripped)
            if record.schema == TRACE_SCHEMA then
                local record_type = record.record_type
                if type(record_type) == "string" then
                    counts[record_type] = (counts[record_type] or 0) + 1
                end
                if is_integer(record.frame_index) then
                    frame_index = record.frame_index
                end
                local payload = record.payload
                if record_type == "render_packet" and type(payload) == "table" then
                    local layout_name = payload.layout_name
                    if type(layout_name) == "string" and layout_name ~= "" then
                        layout_names[layout_name] = true
                    end
                elseif record_type == "semantic_event" and type(payload) == "table" then
                    local category = payload.category or ""
                    local name = payload.name or ""
                    if type(category) == "string" and type(name) == "string" then
                        table.insert(semantic_names, category .. ":" .. name)
                    end
                end
            end
        end
    end
    local layouts = {}
    for name in pairs(layout_names) do
        table.insert(layouts, name)
    end
    table.sort(layouts)
    while #semantic_names > 20 do
        table.remove(semantic_names, 1)
    end
    return {
        frame_index = frame_index,
        record_counts = counts,
        layout_names = layouts,
        semantic_names = semantic_names,
    }
end

function format_summaries(summaries)
    local lines = {"trace\tstatus\tframe_index\temulator\tlines\trender_packet\tsemantic_event\ttop_level"}
    for _, summary in ipairs(summaries) do
        table.insert(lines, table.concat({
            summary.path,
            summary.status,
            tostring(summary.frame_index),
            summary.emulator,
            tostring(summary.lines),
            tostring(summary.render_packet),
            tostring(summary.semantic_event),
            tostring(summary.top_level),
        }, "\t"))
    end
    return table.concat(lines, "\n") .. "\n"
end

function run(args)
    args = args or {}
    local traces = args.traces or {}
    if #traces == 0 then
        raise("validate-trace-ndjson: expected at least one trace path")
    end
    local summaries = {}
    for _, pathname in ipairs(traces) do
        table.insert(summaries, validate_trace(pathname, args))
    end
    io.write(format_summaries(summaries))
    return summaries
end
