local lm = require "luamake"
local fs = require "bee.filesystem"

lm.mode   = "debug"

local function get_path(p, def_p)
    return p and fs.path(p) or def_p
end

EfkRootDir= get_path(lm.EfkDir, fs.path "../")
EfkDir    = EfkRootDir / "Effekseer"
EfkSrc    = EfkDir / "Dev/Cpp"

BgfxDir   = get_path(lm.BgfxDir,    EfkRootDir / "bgfx")
BxDir     = get_path(lm.BxDir,      EfkRootDir / "bx")
BimgDir   = get_path(lm.BimgDir,    EfkRootDir / "bimg")

BgfxBinDir = get_path(lm.BgfxBinDir, BgfxDir / ".build/win64_vs2022/bin")
BgfxNameSuffix = lm.mode == "debug" and "Debug" or "Release"

local function find_shaderc()
    local shaderc = BgfxBinDir / "shaderc.exe"
    if not fs.exists(shaderc) then
        shaderc = BgfxBinDir / ("shaderc%s.exe"):format(BgfxNameSuffix)
        if not fs.exists(shaderc) then
            error(table.concat({
                "need bgfx shaderc tool, tried:",
                (BgfxBinDir / "shaderc.exe"):string(),
                (BgfxBinDir / ("shaderc%s.exe"):format(BgfxNameSuffix)):string()
            }, "\n"))
        end
    end
    return shaderc
end

Shaderc = find_shaderc()

Plat = (function ()
    if lm.os == "windows" then
        if lm.compiler == "gcc" then
            return "mingw"
        end
        return "msvc"
    end
    return lm.os
end)()

function ToStrings(dirs)
    local t = {}
    for _, d in ipairs(dirs) do
        t[#t+1] = d:string()
    end
    return t
end

EfkLib_Includes = ToStrings{
    EfkSrc / "Effekseer",
    EfkSrc,
    BgfxDir / "include",
    BxDir / "include",
}