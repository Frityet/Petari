import("core.base.json")
import("core.project.config")
import("core.project.project")
import("lib.detect.find_tool")

local function sources_for(target)
    local sources = json.mark_as_array({})
    for _, batch in pairs(target:sourcebatches()) do
        for index, source in ipairs(batch.sourcefiles or {}) do
            local object = batch.objectfiles and batch.objectfiles[index]
            if object then
                table.insert(sources, {source = path.relative(path.absolute(source), os.projectdir()), object = path.absolute(object)})
            end
        end
    end
    table.sort(sources, function(a, b) return a.source < b.source end)
    return sources
end

function run(output)
    config.load()
    local targets = json.mark_as_array({})
    local application = assert(project.target("smg-pc"))
    for _, target in ipairs(application:orderdeps()) do
        if target:kind() == "static" then
            table.insert(targets, {name = target:name(), artifact = path.absolute(target:targetfile()), sources = sources_for(target)})
        end
    end
    table.sort(targets, function(a, b) return a.name < b.name end)
    local mapping = path.join(output, "configured-provider-build-map.json")
    json.savefile(mapping, {targets = targets, executable = path.absolute(application:targetfile()), direct_objects = sources_for(application),
        scope = "Configured project archive dependencies and direct executable objects of smg-pc; external package libraries excluded"})
    local python = assert(find_tool("python3"), "python3 is required for provider audit")
    local argv = {path.join(os.projectdir(), "scripts/source_provider_audit.py"), "--root", os.projectdir(), "--build-map", mapping, "--output", output}
    local sdk = config.get("sdk")
    local compiler = assert(project.target("smg-pc-game")):tool("cxx")
    local search_paths = compiler and {path.directory(compiler)} or {}
    if sdk then table.insert(search_paths, path.join(sdk, "bin")) end
    for _, tool in ipairs({"nm", "cxxfilt"}) do
        local found = find_tool("llvm-" .. tool, {paths = search_paths})
        assert(found, "llvm-%s is required for provider audit", tool)
        table.join2(argv, {"--" .. tool, found.program})
    end
    os.vrunv(python.program, argv)
    return json.loadfile(path.join(output, "provider-audit.json"))
end
