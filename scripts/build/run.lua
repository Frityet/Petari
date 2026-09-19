import("core.base.option")
import("core.project.config")
import("devel.debugger")
import("private.action.run.runenvs")

function main(target)
    local args = table.copy(option.get("arguments") or {})
    local explicit_disc = false
    for _, argument in ipairs(args) do
        if argument == "--disc" or argument:startswith("--disc=") then
            explicit_disc = true
        end
    end
    if not explicit_disc then
        local disc = config.get("disc") or os.getenv("SMGPC_DISC_IMAGE")
        if not disc or disc == "" then
            local images = {}
            for _, file in ipairs(os.files(path.join(os.projectdir(), "*"))) do
                if table.contains({".rvz", ".iso", ".wbfs"}, path.extension(file):lower()) then
                    table.insert(images, file)
                end
            end
            assert(#images == 1, "Set your disc once with `xmake f --disc=/path/to/game.rvz`, pass --disc PATH, or put one disc image in the repository root.")
            disc = images[1]
        end
        disc = path.absolute(disc, os.projectdir())
        assert(os.isfile(disc), "Disc image not found: %s", disc)
        table.join2(args, {"--disc", disc})
    end
    local binary = path.absolute(target:targetfile())
    if target:is_plat("macosx") then
        binary = path.join(path.absolute(target:data("xcode.bundle.contentsdir")), "MacOS", path.filename(binary))
    end
    local addenvs, setenvs = runenvs.make(target)
    if option.get("debug") then
        debugger.run(binary, args, {curdir = target:rundir(), addenvs = addenvs, setenvs = setenvs})
    else
        os.execv(binary, args, {curdir = target:rundir(), detach = option.get("detach"), addenvs = addenvs, setenvs = setenvs})
    end
end
