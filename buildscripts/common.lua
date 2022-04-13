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
    local shaderc = BgfxBinDir / ("shaderc%s.exe"):format(BgfxNameSuffix)
    if not fs.exists(shaderc) then
        shaderc = BgfxBinDir / "shaderc.exe"
    end
    return shaderc
end

Shaderc = lm.shaderc or find_shaderc()

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

local stage_mapper<const> = {
    vs = "vs",
    ps = "fs",
}

local shadertype_mapper<const> = {
    unlit       = "Unlit",
    lit         = "Lit",
    distortion  = "BackDistortion",
    ad_unlit    = "AdvancedUnlit",
    ad_lit      = "AdvancedLit",
    ad_distortion="AdvancedBackDistortion",
}

function ShaderInfoFromFilename(filename)
    local fmt = "(%w+)_(%w+)_(%w+)%.fx"
    local adfmt = "ad_" .. fmt
    local modeltype, shadertype, stage = filename:match(adfmt)
    local ad = true
    if modeltype == nil then
        ad = nil
        modeltype, shadertype, stage = filename:match(fmt)
    end

    if modeltype == nil or shadertype == nil or stage == nil then
        error(("invalid filename, 'modeltype should be 'sprite'|'model', shadertype should be 'unlit|lit|distortion', 'stage' shoulde be 'vs|ps', filename is :%s"):format(fn:string()))
    end

    if ad then
        shadertype = "ad_" .. shadertype
    end

    return {
        modeltype   = modeltype,
        shadertype  = shadertype_mapper[shadertype],
        stage       = assert(stage_mapper[stage]),
        filename    = filename,
    }
end