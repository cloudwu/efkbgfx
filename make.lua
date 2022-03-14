local lm = require "luamake"
local fs = require "bee.filesystem"

local efkdir    = fs.path(os.getenv "efkdir" or "../Effekseer")
local efksrc    = efkdir / "Dev/Cpp"

local bgfxdir   = fs.path(os.getenv "bgfxdir" or "../bgfx")
local bxdir     = fs.path(os.getenv "bxdir" or "../bx")
--local bimgdir   = fs.path(os.getenv "bimgdir" or "../bimg")

local function to_strings(dirs)
    local t = {}
    for _, d in ipairs(dirs) do
        t[#t+1] = d:string()
    end
    return t
end

local efklib_includes = to_strings{
    efksrc / "Effekseer",
    efksrc,
    bgfxdir / "include",
    bxdir / "include",
}

lm:source_set "source_efklib" {
    includes = efklib_includes,
    sources = to_strings{
        efksrc / "Effekseer/Effekseer/**.cpp",
        efksrc / "EffekseerMaterial/*.cpp",
        efksrc / "EffekseerRendererCommon/*.cpp",
    },
    defines = "BX_CONFIG_DEBUG=" .. (lm.mode == "debug" and 1 or 0),
}

lm:source_set "source_efkbgfx" {
    includes = {
        efklib_includes,
        "./renderer"
    },
    sources = {
        "renderer/*.cpp",
    },
    defines = "BX_CONFIG_DEBUG=" .. (lm.mode == "debug" and 1 or 0),
}

lm:dll "efklib" {
    deps = {
        "source_efklib",
        "source_efkbgfx",
    },

}
