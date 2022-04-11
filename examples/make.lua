local lm = require "luamake"
local fs = require "bee.filesystem"

package.path = "./?.lua;../?.lua"

lm.mode = "debug"
lm.EfkDir = "../../"
lm.BgfxBinDir = "../../bgfx/.build/win64_vs2022/bin"
require "buildscripts.common"

lm.builddir = ("build/%s/%s"):format(Plat, lm.mode)
lm.bindir = ("bin/%s/%s"):format(Plat, lm.mode)

if not fs.exists(Shaderc) then
    error(("shaderc:%s is not exist!"):format(Shaderc))
end

local bx_libname    = "bx" .. BgfxNameSuffix
local bgfx_libname  = "bgfx" .. BgfxNameSuffix
local bimg_libname  = "bimg" .. BgfxNameSuffix
local bimgDecode_libname = "bimg_decode" .. BgfxNameSuffix
local bgfx_example_dir = BgfxDir / "examples"
local alloca_file_includes = {
    msvc = BxDir / "include/compat/msvc",
    mingw = BxDir / "include/compat/mingw",
}

if not lm.StaticBgfxLib then
    local bgfxdll_name  = lm.BgfxSharedDll or ("bgfx-shared-lib" .. BgfxNameSuffix .. ".dll")
    lm:copy "copy_bgfx" {
        input   = (BgfxBinDir / bgfxdll_name):string(),
        output  = lm.bindir .. "/" .. bgfxdll_name,
    }
end

lm:import "../efkmatc/make.lua"
lm:import "../renderer/make.lua"
lm:import "../shaders/make.lua"

lm:exe "example"{
    deps = {
        "efklib",
        "efkbgfx",
        "efkmat",
        "copy_bgfx",
    },
    includes = {
        alloca_file_includes[Plat]:string(),
        EfkLib_Includes,
        (bgfx_example_dir / "common"):string(),
        (BimgDir / "include"):string(),
        "../",
    },
    sources = {
        "example.cpp",
    },
    defines = {
        "BX_CONFIG_DEBUG=" .. (lm.mode == "debug" and 1 or 0),
        "ENTRY_CONFIG_IMPLEMENT_MAIN=1",
    },
    links = {
        "example-common" .. BgfxNameSuffix,
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
        BgfxBinDir:string(),
    }
}

----------------------------------------------------------------
local cwd = fs.path(lm.workdir)
local sc = require "buildscripts.shader_compile"
local cube_shader_dir = fs.path "cube/shaders"
for _, s in ipairs{
    "vs_fullscreen.sc",
    "fs_fullscreen.sc",
    "vs_cube.sc",
    "fs_cube.sc",
} do
    local input = cube_shader_dir / s
    local output = fs.path(input):replace_extension "bin"
    local stage = s:match "([vf]s)"
    local cfg = {
        stage = stage,
        Plat = lm.os,
        optimizelevel = 3,
        debug = true,
        includes = {
            cwd / BgfxDir / "src",
            cwd / bgfx_example_dir / "common",
        },
        defines = {},
        input = input:string(),
        output = output:string(),
    }
    local cmd = sc.gen_cmd(Shaderc:string(), cfg)
    lm:build(cmd)
end