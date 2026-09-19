import("core.base.json")
import("core.base.option")
import("common")

local function git(repository, args)
    -- Porcelain -z output must not be auto-detected as UTF-16 because of NULs.
    local output = os.tmpfile()
    return try {
        function ()
            os.vrunv("git", args, {curdir = repository, stdout = output})
            return io.readfile(output, {encoding = "binary"})
        end,
        finally {function (ok, errors)
            os.tryrm(output)
            if not ok then raise(errors) end
        end}
    }
end

local function snapshot(repository, scopes)
    local result = {repository = repository, head = git(repository, {"rev-parse", "HEAD"}):trim(), scopes = scopes,
                    dirty_paths = json.mark_as_array({}), submodules = json.mark_as_array({})}
    local entries = git(repository, table.join({"status", "--porcelain=v1", "-z", "--untracked-files=all", "--ignore-submodules=none", "--"}, scopes)):split("\0", {plain = true})
    local index = 1
    while index <= #entries do
        local entry = entries[index]
        if entry ~= "" then
            local status, name = entry:sub(1, 2), entry:sub(4)
            local record = {status = status, path = name}
            if status:find("[RC]") then index = index + 1; record.original_path = entries[index] end
            local file = path.join(repository, name)
            if os.islink(file) then record.symlink_target = os.readlink(file)
            elseif os.isfile(file) then record.sha256 = hash.sha256(file); record.size = os.filesize(file)
            else record.kind = os.isdir(file) and "directory_or_submodule" or "deleted_or_missing" end
            table.insert(result.dirty_paths, record)
        end
        index = index + 1
    end
    for entry in git(repository, table.join({"ls-files", "--stage", "-z", "--"}, scopes)):gmatch("([^%z]+)") do
        local commit, stage, name = entry:match("^160000 (%x+) (%d+)\t(.+)$")
        if name then
            local child = {path = name, index_commit = commit, index_stage = stage}
            local directory = path.join(repository, name)
            if os.exists(path.join(directory, ".git")) then child.checkout = snapshot(directory, {"."})
            else child.initialized = false end
            table.insert(result.submodules, child)
        end
    end
    return result
end

local function requirements(binary, allow_external)
    local file = assert(io.open(binary, "rb"))
    local header = file:read(16)
    file:close()
    assert(header and header:sub(1, 8) == "\207\250\237\254\12\0\0\1" and header:sub(13, 16) == "\2\0\0\0", "binary must be a native arm64 Mach-O executable")
    local commands = os.iorunv("/usr/bin/otool", {"-l", binary})
    local minimum = commands:match("cmd LC_BUILD_VERSION.-\n%s*minos (%S+)") or commands:match("cmd LC_VERSION_MIN_MACOSX.-\n%s*version (%S+)")
    assert(minimum, "could not determine minimum macOS version")
    local libraries, external = {}, {}
    local listing = os.iorunv("/usr/bin/otool", {"-L", binary})
    for line in listing:gmatch("\n([^\n]+)") do
        local library = line:match("^%s*(.-) %(compatibility version")
        if library then
            table.insert(libraries, library)
            if not library:startswith("/System/Library/") and not library:startswith("/usr/lib/") then table.insert(external, library) end
        end
    end
    assert(allow_external or #external == 0, "Unbundled dependencies: %s. Use --source-app to preserve an existing app and its dependencies.", table.concat(external, ", "))
    os.vrunv("/usr/bin/codesign", {"--verify", binary})
    return {architecture = "arm64", minimum_macos = minimum, linked_libraries = libraries, external_libraries = external,
            binary_signature_verified = true, bundle_signing = "Local development bundle; no distribution signing or notarization"}
end

local function xml(value)
    return tostring(value):gsub("&", "&amp;"):gsub("<", "&lt;"):gsub(">", "&gt;"):gsub('"', "&quot;")
end

function main()
    assert(is_host("macosx"), "Local demo packaging requires macOS")
    local source = option.get("source-app")
    local binary = option.get("binary")
    assert(not (source and binary), "Choose --binary or --source-app")
    local executable = "smg-pc"
    if source then
        source = path.absolute(source)
        executable = os.iorunv("/usr/libexec/PlistBuddy", {"-c", "Print :CFBundleExecutable", path.join(source, "Contents/Info.plist")}):trim()
        assert(executable:match("^[%w_.-]+$") and not executable:startswith("launch-"), "Source app must contain a plain game executable")
        binary = path.join(source, "Contents/MacOS", executable)
    end
    binary = path.absolute(binary or common.targetfile("smg-pc"))
    local disc = path.absolute(assert(option.get("disc"), "--disc is required"))
    local expected = assert(option.get("expected-sha256"), "--expected-sha256 pins the selected executable"):lower()
    assert(#expected == 64 and expected:match("^%x+$"), "Expected SHA256 must contain 64 hexadecimal digits")
    assert(os.isfile(binary) and os.isexec(binary), "Binary is absent or not executable: %s", binary)
    assert(os.isfile(disc), "Disc image not found: %s", disc)
    assert(hash.sha256(binary) == expected, "Binary differs from --expected-sha256")
    local name = option.get("name") or "Super Mario Galaxy Movement Demo"
    assert(name:trim() ~= "" and not name:find("[/\\\r\n%z]"), "Invalid application name")
    local output = path.absolute(option.get("output") or path.join("build/playable-demo", name .. ".app"))
    assert(output:endswith(".app") and not os.exists(output) and not os.islink(output), "Choose a new output path ending in .app: %s", output)
    local platform = requirements(binary, source ~= nil)
    local note = option.get("validation-note")
    local validation = note and io.readfile(note) or "No runtime validation report supplied. Packaging does not establish working movement or full gameplay."
    local provenance = {
        schema_version = 1, app_name = name, packaged_at_utc = os.date("!%Y-%m-%dT%H:%M:%SZ"),
        source_binary = binary, source_app = source, binary_sha256 = expected, binary_size = os.filesize(binary),
        output_app = output, external_disc = disc, bundled_game_assets = false,
        launch_arguments = {"--stage", "HeavensDoorGalaxy", "--scenario", "1", "--disc", disc},
        platform = platform, source_checkout_at_packaging = snapshot(os.projectdir(), {"src", "tests", "scripts", "xmake.lua", ".gitmodules", "aurora", "decomp"}),
        provenance_limit = "Packaging-time checkout snapshot, not a build attestation. No runtime validation is performed by the packager.",
        runtime_validation_report = note and {path = path.absolute(note), sha256 = hash.sha256(note), description = "Caller-supplied report"} or nil
    }
    if option.get("dry-run") then print(json.encode(provenance)); return end
    os.mkdir(path.directory(output))
    local temporary = path.join(path.directory(output), ".demo-" .. hash.uuid4())
    local staged = path.join(temporary, path.filename(output))
    try {
        function ()
            local macos, resources = path.join(staged, "Contents/MacOS"), path.join(staged, "Contents/Resources")
            os.mkdir(macos, resources)
            local copied, launchpath
            if source then
                -- Keep the source app's plist and signature together. Changing
                -- its plist would invalidate the unchanged Mach-O's signature.
                local payload = path.join(resources, "Engine.app")
                os.cp(source, payload)
                copied = path.join(payload, "Contents/MacOS", executable)
                launchpath = "../Resources/Engine.app/Contents/MacOS/" .. executable
            else
                copied = path.join(macos, executable)
                launchpath = executable
                os.cp(binary, copied)
                os.vrunv("chmod", {"755", copied})
            end
            local launcher = io.readfile(path.join(os.projectdir(), "scripts/package/launcher.sh.in"))
            launcher = launcher:gsub("@DISC@", function () return "'" .. disc:gsub("'", "'\"'\"'") .. "'" end)
            launcher = launcher:gsub("@EXECUTABLE@", function () return launchpath end)
            local launchfile = path.join(macos, "launch-demo")
            io.writefile(launchfile, launcher)
            os.vrunv("chmod", {"755", launchfile})
            os.vrunv("/bin/sh", {"-n", launchfile})
            local info = {CFBundleInfoDictionaryVersion = "6.0", CFBundleExecutable = "launch-demo",
                CFBundleIdentifier = "org.petari.smg-pc." .. name:lower():gsub("[^a-z0-9-]", "-"),
                CFBundleName = name, CFBundleDisplayName = name, CFBundlePackageType = "APPL",
                CFBundleShortVersionString = "0.1.0", CFBundleVersion = "1", LSMinimumSystemVersion = platform.minimum_macos,
                NSPrincipalClass = "NSApplication", SMGPCBinarySHA256 = expected}
            local plist = {'<?xml version="1.0" encoding="UTF-8"?>', '<plist version="1.0"><dict>'}
            for key, value in pairs(info) do table.insert(plist, "<key>" .. key .. "</key><string>" .. xml(value) .. "</string>") end
            table.insert(plist, '<key>NSHighResolutionCapable</key><true/></dict></plist>')
            io.writefile(path.join(staged, "Contents/Info.plist"), table.concat(plist, "\n"))
            io.writefile(path.join(resources, "README.txt"), name .. "\n\nRequires macOS " .. platform.minimum_macos .. " on Apple Silicon.\nExternal disc: " .. disc .. "\n\nWASD: movement. Space/Enter/left mouse: A. Arrows: camera. C: reset camera. Z: crouch. X: spin. F9: development free camera.\nLogs: ~/Library/Logs/PetariDemo/\n\n" .. validation .. "\n")
            if note then io.writefile(path.join(resources, "runtime-validation.txt"), validation) end
            json.savefile(path.join(resources, "provenance.json"), provenance)
            os.vrunv("/usr/bin/plutil", {"-lint", path.join(staged, "Contents/Info.plist")})
            assert(hash.sha256(copied) == expected and hash.sha256(binary) == expected, "Executable changed during packaging")
            os.vrunv("/usr/bin/codesign", {"--verify", copied})
            assert(not os.exists(output) and not os.islink(output), "Output appeared during packaging")
            os.mv(staged, output)
        end,
        finally {function (ok, errors)
            os.tryrm(temporary)
            if not ok then raise(errors) end
        end}
    }
    if option.get("manifest") then common.write_json(option.get("manifest"), provenance) end
    print("Packaged %s", output)
end
