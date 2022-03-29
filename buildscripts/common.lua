local lm = require "luamake"
local fs = require "bee.filesystem"

lm.mode   = "debug"

EfkRootDir= fs.path(lm.EfkDir or "../")
EfkDir    = EfkRootDir / "Effekseer"
EfkSrc    = EfkDir / "Dev/Cpp"

BgfxDir   = EfkRootDir / "bgfx"
BxDir     = EfkRootDir / "bx"
BimgDir   = EfkRootDir / "bimg"

BgfxBinDir   = BgfxDir / ".build/win64_vs2022/bin"
BgfxNameSuffix   = lm.mode == "debug" and "Debug" or "Release"
Shaderc = BgfxBinDir / ("shaderc%s.exe"):format(BgfxNameSuffix)

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