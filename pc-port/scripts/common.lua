import("core.base.json")
import("core.base.process")
import("lib.detect.find_tool")

local function tostring_or_empty(value)
    if value == nil then
        return ""
    end
    return tostring(value)
end

function project_root()
    return os.projectdir()
end

function repo_root()
    return path.directory(project_root())
end

function env_value(name, default, env)
    local value = env and env[name] or os.getenv(name)
    if value == nil or value == "" then
        return default
    end
    return value
end

function env_bool(name, default, env)
    local value = env_value(name, nil, env)
    if value == nil then
        return default
    end
    value = tostring(value):lower()
    return value == "1" or value == "true" or value == "yes" or value == "on"
end

function env_number(name, default, env)
    local value = env_value(name, nil, env)
    if value == nil then
        return default
    end
    local number = tonumber(value)
    if number == nil then
        raise("%s must be numeric, got %s", name, value)
    end
    return number
end

function make_env(overrides)
    local env = {}
    for key, value in pairs(os.getenvs() or {}) do
        env[key] = value
    end
    for key, value in pairs(overrides or {}) do
        if value == nil then
            env[key] = nil
        else
            env[key] = tostring(value)
        end
    end
    return env
end

function env_array(env)
    local out = {}
    for key, value in pairs(env or {}) do
        if value ~= nil then
            table.insert(out, tostring(key) .. "=" .. tostring(value))
        end
    end
    table.sort(out)
    return out
end

function command_text(program, argv)
    local parts = {program}
    for _, arg in ipairs(argv or {}) do
        local text = tostring_or_empty(arg)
        if text:find("[^%w%-%._/:=,@+]") then
            text = "'" .. text:gsub("'", [['"'"']]) .. "'"
        end
        table.insert(parts, text)
    end
    return table.concat(parts, " ")
end

function require_tool(name)
    local tool = find_tool(name)
    if not tool or not tool.program then
        raise("missing required tool `%s`", name)
    end
    return tool.program
end

function file_exists(pathname)
    return pathname ~= nil and os.isfile(pathname)
end

function file_size(pathname)
    if file_exists(pathname) then
        return os.filesize(pathname)
    end
    return 0
end

function file_nonempty(pathname)
    return file_size(pathname) > 0
end

function file_state(pathname)
    if file_nonempty(pathname) then
        return "present"
    end
    return "missing"
end

function read_file(pathname, binary)
    return io.readfile(pathname, binary and {encoding = "binary"} or nil)
end

function write_file(pathname, text)
    os.mkdir(path.directory(pathname))
    io.writefile(pathname, text)
end

function write_json(pathname, value)
    write_file(pathname, json.encode(value) .. "\n")
end

function load_json(pathname)
    return json.decode(read_file(pathname) or "")
end

function tail(pathname, lines)
    if not file_exists(pathname) then
        return ""
    end
    local text = read_file(pathname) or ""
    local out = {}
    for line in text:gmatch("([^\r\n]*)\r?\n") do
        table.insert(out, line)
        if #out > lines then
            table.remove(out, 1)
        end
    end
    if #out == 0 and text ~= "" then
        return text
    end
    return table.concat(out, "\n")
end

function read_binary_contains(pathname, needle)
    if not file_exists(pathname) then
        return false
    end
    local text = read_file(pathname, true) or ""
    return text:find(needle, 1, true) ~= nil
end

function runv(program, argv, opt)
    opt = opt or {}
    local log_file
    local stdout = opt.stdout
    local stderr = opt.stderr
    if type(stdout) == "string" and stdout == stderr then
        os.mkdir(path.directory(stdout))
        log_file = io.open(stdout, "wb")
        if not log_file then
            raise("could not open log %s", stdout)
        end
        stdout = log_file
        stderr = log_file
    end
    local execopt = {
        curdir = opt.curdir,
        envs = opt.env and env_array(opt.env) or nil,
        stdout = stdout,
        stderr = stderr,
    }
    if opt.env_extra then
        execopt.envs = env_array(make_env(opt.env_extra))
    end
    print("+ " .. command_text(program, argv))
    local proc = process.openv(program, argv or {}, execopt)
    local done, status = proc:wait(opt.timeout_ms or -1)
    proc:close()
    if log_file then
        log_file:close()
    end
    if done ~= 1 or status ~= 0 then
        if opt.check == false then
            return false, status or done
        end
        raise("command failed (%s): %s", tostring(status or done), command_text(program, argv))
    end
    return true, status
end

function capturev(program, argv, opt)
    opt = opt or {}
    local stdout = os.tmpfile()
    local stderr = os.tmpfile()
    local ok, status = runv(program, argv, {
        curdir = opt.curdir,
        env = opt.env,
        env_extra = opt.env_extra,
        stdout = stdout,
        stderr = stderr,
        check = false,
        timeout_ms = opt.timeout_ms,
    })
    local out = read_file(stdout) or ""
    local err = read_file(stderr) or ""
    os.tryrm(stdout)
    os.tryrm(stderr)
    if not ok and opt.check ~= false then
        raise("command failed (%s): %s\n%s%s", tostring(status), command_text(program, argv), out, err)
    end
    return out, err, ok, status
end

function open_process(program, argv, opt)
    opt = opt or {}
    local log_file
    local stdout = opt.stdout
    local stderr = opt.stderr
    if opt.log then
        os.mkdir(path.directory(opt.log))
        log_file = io.open(opt.log, "wb")
        if not log_file then
            raise("could not open log %s", opt.log)
        end
        stdout = log_file
        stderr = log_file
    end
    print("+ " .. command_text(program, argv))
    local proc = process.openv(program, argv or {}, {
        curdir = opt.curdir,
        envs = opt.env and env_array(opt.env) or nil,
        stdout = stdout,
        stderr = stderr,
    })
    return proc, log_file
end

function close_process(proc, log_file)
    if proc then
        proc:close()
    end
    if log_file then
        log_file:close()
    end
end

function kill_process(proc)
    if proc then
        proc:kill()
    end
end

function wait_process(proc, timeout_seconds)
    local deadline = os.mclock() + timeout_seconds * 1000
    while os.mclock() < deadline do
        local done, status = proc:wait(100)
        if done == 1 then
            return status == 0, status
        end
    end
    return false, "timeout"
end

function split_words(text)
    local out = {}
    for word in tostring(text or ""):gmatch("%S+") do
        table.insert(out, word)
    end
    return out
end

function split_commas_or_table(value)
    if value == nil then
        return {}
    end
    if type(value) == "table" then
        return value
    end
    local out = {}
    for part in tostring(value):gmatch("[^,]+") do
        part = part:gsub("^%s+", ""):gsub("%s+$", "")
        if part ~= "" then
            table.insert(out, part)
        end
    end
    return out
end

function parse_key_value(value)
    local key, val = tostring(value or ""):match("^([^=]+)=(.*)$")
    return key, val
end

function start_xvfb(width, height, display, log_path)
    local xvfb = require_tool("Xvfb")
    local chosen = display
    if not chosen then
        for candidate = 130, 229 do
            if not os.isfile("/tmp/.X11-unix/X" .. tostring(candidate)) then
                chosen = candidate
                break
            end
        end
    end
    if not chosen then
        raise("could not find a free X11 display number in :130..:229")
    end

    local proc, log = open_process(xvfb, {
        ":" .. tostring(chosen),
        "-screen",
        "0",
        string.format("%dx%dx24", width, height),
        "-nolisten",
        "tcp",
    }, {log = log_path})
    os.sleep(500)
    local done, status = proc:wait(1)
    if done == 1 then
        close_process(proc, log)
        raise("Xvfb exited early with status %s, see %s", tostring(status), log_path)
    end
    return {
        display = chosen,
        proc = proc,
        log = log,
    }
end

function stop_xvfb(server)
    if not server then
        return
    end
    kill_process(server.proc)
    close_process(server.proc, server.log)
end

