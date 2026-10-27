local fusionui_root = os.scriptdir()
local fusionui_standalone = os.projectdir() == fusionui_root

if fusionui_standalone then
    includes(os.getenv("COMMONLIBF4_DIR") or "lib/commonlibf4")
    includes(path.join(os.getenv("FUSIONFRAMEWORK_DIR") or "extern/fusion-framework-fo4", "xmake.lua"))
    set_project("fusion-ui-fo4")
    set_languages("c++23")
    set_warnings("allextra")
    add_rules("mode.debug", "mode.releasedbg")
    set_defaultmode("releasedbg")
end

add_requires("imgui v1.92.7", { configs = { dx11 = true } })
add_requires("nlohmann_json")
add_requires("stb")

local function fusionui_dirs()
    add_includedirs(path.join(fusionui_root, "include"), path.join(fusionui_root, "src"))
    add_defines("NOMINMAX")
end

rule("fusionui.version")
    on_load(function (target)
        local version = target:values("fusionui.version")
        local file = target:values("fusionui.version_file")
        if not version and file and os.isfile(file) then
            version = io.readfile(file):match(target:values("fusionui.version_pattern"))
        end
        version = version or target:version()
        if version then target:add("defines", 'FUSIONUI_MOD_VERSION="' .. version .. '"') end
    end)
rule_end()

local fusionui_game_sources = {"src/core/*.cpp", "src/client/*.cpp", "src/kit/*.cpp", "src/game/*.cpp", "src/modern/*.cpp",
    "src/fo4/*.cpp", "src/styles/*/*.cpp", "src/screens/*.cpp", "src/perf/*.cpp"}

rule("fusionui.map-icons")
    before_build(function ()
        for _, file in ipairs({"map.json", "map.dds"}) do
            assert(os.isfile(path.join(fusionui_root, "assets/icons", file)),
                "Required runtime map atlas is missing from this source snapshot" .. fusionui_root)
        end
    end)
rule_end()

target("FusionUI")
    add_rules("fusion.version_header")
    set_values("fusion.version_header", path.join(fusionui_root, "include/fusionui/Version.h"), "FUSIONUI_VERSION")
    add_rules("commonlibf4.plugin", {
        name = "FusionUI",
        author = "B21",
        description = "Native ImGui UI framework for B21 F4SE plugins"
    })
    add_rules("fusion.exports", { prefixes = { "fusionui::", "Im" }, packages = { "imgui" } })
    add_rules("fusionui.map-icons")
    set_default(fusionui_standalone)
    fusionui_dirs()
    for _, pattern in ipairs(fusionui_game_sources) do add_files(path.join(fusionui_root, pattern)) end
    add_packages("imgui", "nlohmann_json", "stb")
    fusionframework_use()
    add_syslinks("d3d11", "d3dcompiler", "dxgi", "user32", "psapi", "shell32", "ole32", "version", "shcore")
    add_shflags("/PDBALTPATH:FusionUI.pdb", { force = true })
    add_shflags("/PDBSTRIPPED:$(builddir)/FusionUI.stripped.pdb", { force = true })
    add_installfiles(path.join(fusionui_root, "assets/fonts/*.ttf"), path.join(fusionui_root, "assets/fonts/*.otf"),
        path.join(fusionui_root, "assets/fonts/*.txt"), { prefixdir = "F4SE/Plugins/FusionUI/fonts" })
    for _, pack in ipairs({"fusion", "fo76", "fo4", "skyrim", "starfield", "fnv", "map"}) do
        add_installfiles(path.join(fusionui_root, "assets/icons/" .. pack .. ".json"), path.join(fusionui_root, "assets/icons/" .. pack .. ".dds"),
            { prefixdir = "F4SE/Plugins/FusionUI/icons" })
    end
    add_installfiles(path.join(fusionui_root, "assets/crosshairs/builtin.json"), { prefixdir = "F4SE/Plugins/FusionUI/crosshairs" })
    after_build(function (target)
        if os.projectdir() ~= fusionui_root then return end
        os.tryrm(path.join(fusionui_root, "F4SE"))
        local srcfiles, dstfiles = target:installfiles(fusionui_root)
        for i, src in ipairs(srcfiles or {}) do os.cp(src, dstfiles[i]) end
        local plugins = path.join(fusionui_root, "F4SE/Plugins")
        local framework = target:dep("FusionFramework")
        os.cp(framework:targetfile(), plugins .. "/")
        if os.isfile(framework:symbolfile()) then os.cp(framework:symbolfile(), plugins .. "/") end
    end)
target_end()

function fusionui_use()
    fusionui_dirs()
    add_rules("fusionui.version")
    add_deps("FusionUI")
    add_packages("imgui", { links = {} })
    add_packages("nlohmann_json", "stb")
    add_defines("GImGui=(ImGui::GetCurrentContext())")
    add_shflags("/DELAYLOAD:FusionUI.dll", { force = true })
end

