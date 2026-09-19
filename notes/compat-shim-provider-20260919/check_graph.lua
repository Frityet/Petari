import("core.project.project")
import("core.project.config")
import("core.base.json")
config.load()
local directory = path.join(os.projectdir(), "notes/compat-shim-provider-20260919")
local affected = json.loadfile(path.join(directory, "affected-targets.json"))
local rows = json.mark_as_array({})
local all_ok = true
for _, original in ipairs(affected) do
    local target = assert(project.target(original.target), original.target)
    local dependencies = json.mark_as_array({})
    local base = false
    for _, dependency in ipairs(target:orderdeps()) do
        table.insert(dependencies, dependency:name())
        if dependency:name() == "aurora-base" then base = true end
    end
    local direct_shim = false
    for _, source in ipairs(target:sourcefiles()) do
        if source:endswith("aurora/lib/compat.cpp") then direct_shim = true end
    end
    local command = target:linkcmd()
    local direct_object = command:find("aurora/lib/compat.cpp.o", 1, true) ~= nil
    local direct_objects = json.mark_as_array({})
    for _, object in ipairs(target:objectfiles()) do
        table.insert(direct_objects, object)
        if object:endswith("aurora/lib/compat.cpp.o") then direct_object = true end
    end
    local row = {target = original.target, build_file = original.build_file, dependencies = dependencies,
                 has_aurora_base = base, direct_shim_source = direct_shim, direct_shim_object = direct_object,
                 direct_objects = direct_objects, link_command = command}
    table.insert(rows, row)
    if not base or direct_shim or direct_object then all_ok = false; print("FAIL " .. original.target) end
end
local base_sources = json.mark_as_array({})
for _, source in ipairs(assert(project.target("aurora-base")):sourcefiles()) do
    if source:endswith("aurora/lib/compat.cpp") then table.insert(base_sources, source) end
end
local owners = json.mark_as_array({})
for name, target in pairs(project.targets()) do
    for _, source in ipairs(target:sourcefiles()) do
        if source:endswith("aurora/lib/compat.cpp") then table.insert(owners, name) end
    end
end
all_ok = all_ok and #base_sources == 1 and #owners == 1 and owners[1] == "aurora-base"
json.savefile(path.join(directory, "configured-shim-dependencies.json"), {all_ok = all_ok, targets = rows, base_shim_sources = base_sources, all_configured_shim_owners = owners})
assert(all_ok, "every removed direct shim must have exactly the Aurora base dependency and no stale direct link object")
print("PASS " .. #rows .. " affected targets retain Aurora base and exclude direct shim objects")
