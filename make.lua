local lm = require "luamake"
local fs = require "bee.filesystem"

local efkdir    = fs.path(os.getenv "efkdir" or "../Effekseer")
local efksrc    = efkdir / "Dev/Cpp"

local bgfxdir   = fs.path(os.getenv "bgfxdir" or "../bgfx")
local bxdir     = fs.path(os.getenv "bxdir" or "../bx")
local bimgdir   = fs.path(os.getenv "bimgdir" or "../bimg")

local plat = (function ()
    if lm.os == "windows" then
        if lm.compiler == "gcc" then
            return "mingw"
        end
        return "msvc"
    end
    return lm.os
end)()

lm.mode = "debug"
lm.builddir = ("build/%s/%s"):format(plat, lm.mode)
lm.bindir = ("bin/%s/%s"):format(plat, lm.mode)

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
    defines = {
        "BX_CONFIG_DEBUG=" .. (lm.mode == "debug" and 1 or 0),
        "EFXBGFX_EXPORTS=1",
    }
}

local name_suffix = lm.mode == "debug" and "Debug" or "Release"
local bgfxbin_dir = bgfxdir / ".build/win64_vs2022/bin"
local bgfx_shared_libname = "bgfx-shared-lib" .. name_suffix
local bgfxdll_name = bgfx_shared_libname .. ".dll"
local bgfxdll = bgfxbin_dir / bgfxdll_name

lm:copy "copy_bgfx" {
    input = bgfxdll:string(),
    output = "build/bin/" .. bgfxdll_name
}

lm:dll "efklib" {
    deps = {
        "source_efklib",
        "source_efkbgfx",
        "copy_bgfx",
    },
}

--------------------------------------
local bx_libname = "bx" .. name_suffix
local bgfx_libname = "bgfx" .. name_suffix
local bimg_libname = "bimg" .. name_suffix
local bimgDecode_libname = "bimg_decode" .. name_suffix
local bgfx_example_dir = bgfxdir / "examples"
local alloca_file_includes = {
    msvc = bxdir / "include/compat/msvc",
    mingw = bxdir / "include/compat/mingw",
}
lm:exe "example"{
    deps = "efklib",
    includes = {
        alloca_file_includes[plat]:string(),
        efklib_includes,
        (bgfx_example_dir / "common"):string(),
        (bimgdir / "include"):string(),
    },
    sources = {
        "examples/example.cpp",
    },
    defines = {
        "BX_CONFIG_DEBUG=" .. (lm.mode == "debug" and 1 or 0),
        "ENTRY_CONFIG_IMPLEMENT_MAIN=1",
    },
    links = {
        "example-common" .. name_suffix,
        bx_libname,
        bimg_libname,
        bgfx_libname,
        bimgDecode_libname,
        "DelayImp",
        "gdi32",
        "psapi",
        "kernel32",
        "user32",
        "winspool",
        "comdlg32",
        "advapi32",
        "shell32",
        "ole32",
        "oleaut32",
        "uuid",
        "odbc32",
        "odbccp32"
    },
    linkdirs = {
        bgfxbin_dir:string(),
    }
}