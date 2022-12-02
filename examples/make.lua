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

local bgfxdll_name  = lm.BgfxSharedDll or ("bgfx-shared-lib" .. BgfxNameSuffix .. ".dll")
lm:copy "copy_bgfx" {
    input   = (BgfxBinDir / bgfxdll_name):string(),
    output  = lm.bindir .. "/" .. bgfxdll_name,
}

lm:import "../renderer/make.lua"
lm:import "../shaders/make.lua"

----------------------------------------------------------------
local cwd = fs.path(lm.workdir)
local sc = require "buildscripts.shader_compile"
local cube_shader_dir = fs.path "cube/shaders"

local function tosc(name)
    return {
        input = cwd / cube_shader_dir / name,
        stage = name:match "([vf]s)",
    }
end

local scfiles = {
    tosc "vs_fullscreen.sc",
    tosc "fs_fullscreen.sc",
    tosc "vs_cube.sc",
    tosc "fs_cube.sc",
}

local shader_folder = cwd / "../shaders"
for fn in fs.pairs(shader_folder) do
    if not fn:string():match "varying.def.sc" then
        local ext = fn:extension():string():lower()
        if ext == ".sc" then
            local si = ShaderInfoFromFilename(fn:filename():string())
            scfiles[#scfiles+1] = {
                input = shader_folder / si.filename,
                stage = si.stage,
                varying_path = shader_folder / ("%s_%s_varying.def.sc"):format(si.modeltype, si.shadertype),
            }
        end
    end
end

local shaderbin_files = {}
for _, f in ipairs(scfiles) do
    local input = f.input
    local output = fs.path(input):replace_extension "bin"
    local cfg = {
        stage = f.stage,
        optimizelevel = 3,
        debug = true,
        varying_path = f.varying_path,
        includes = {
            cwd / BgfxDir / "src",
            cwd / bgfx_example_dir / "common",
        },
        defines = {
            --NOTICE: only sRGB texture and framebuffer mode should add this macro
            --LINEAR_INPUT_COLOR=1,
        },
        input = input:string(),
        output = output:string(),
    }
    shaderbin_files[#shaderbin_files+1] = output:string()
    local cmd = sc.gen_cmd(Shaderc:string(), cfg)
    cmd.description = "Compile shader bin: $out"
    lm:build(cmd)
end

lm:phony "shader_binaries" {
    deps = "efxbgfx_shaders",
    input = shaderbin_files,
}

--------------------------example
lm:exe "example"{
    deps = {
        "efklib",
        "efkbgfx",
        "efkmat",
        "shader_binaries",
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