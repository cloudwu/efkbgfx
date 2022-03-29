local lm = require "luamake"
local fs = require "bee.filesystem"

local smdir     = fs.path(lm.EfkDir or "../")
local efkdir    = smdir / "Effekseer"
local efksrc    = efkdir / "Dev/Cpp"

local bgfxdir   = smdir / "bgfx"
local bxdir     = smdir / "bx"
local bimgdir   = smdir / "bimg"

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
    defines = {
        lm.mode == "debug" and "_DEBUG_EFFEKSEER=1" or nil
    }
}

local MaxInstanced<const> = 20

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
        "MaxInstanced=" .. MaxInstanced,
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

lm:lib "efklib" {
    deps = "source_efklib",
}

lm:dll "efkbgfx" {
    deps = {
        "efklib",
        "source_efkbgfx",
        "copy_bgfx",
    },
}

--------------------------------------
lm:lua_dll "efkmat" {
    includes = {
        efklib_includes,
    },
    sources = {
        "efkmatc/efkmat.cpp",
    },
    deps = "source_efklib",
    bindir = "efkmatc",
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
    deps = {
        "efklib",
        "efkbgfx",
        "efkmat",
    },
    includes = {
        alloca_file_includes[plat]:string(),
        efklib_includes,
        (bgfx_example_dir / "common"):string(),
        (bimgdir / "include"):string(),
        "./",
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

local platform_renderers = {
    windows = "direct3d11",
    ios = "metal",
    macos = "metal",
    linux = "vulkan",
    android = "vulkan",
}

local cwd = fs.path(lm.workdir)
local vulkan_shader_dir = cwd / "../Effekseer/Dev/Cpp/EffekseerRendererVulkan/EffekseerRenderer/Shader"
local shader_output_dir = cwd / "examples/shaders"

fs.create_directories(shader_output_dir)

local shaderfiles = {
    sprite = {
        Unlit = {
            vs      = "sprite_unlit_vs.fx.vert",
            fs      = "model_unlit_ps.fx.frag",
        },
        Lit = {
            vs      = "sprite_lit_vs.fx.vert",
            fs      = "model_lit_ps.fx.frag",
        },
        BackDistortion = {
            vs      = "sprite_distortion_vs.fx.vert",
            fs      = "model_distortion_ps.fx.frag",
        },
        AdvancedUnlit = {
            vs      = "ad_sprite_unlit_vs.fx.vert",
            fs      = "ad_model_unlit_ps.fx.frag",
        },
        AdvancedLit = {
            vs      = "ad_sprite_lit_vs.fx.vert",
            fs      = "ad_model_lit_ps.fx.frag",
        },
        AdvancedBackDistortion = {
            vs      = "ad_sprite_distortion_vs.fx.vert",
            fs      = "ad_model_distortion_ps.fx.frag",
        },
    },
    model = {
        Unlit = {
            vs      = "model_unlit_vs.fx.vert",
            fs      = "model_unlit_ps.fx.frag",
        },
        Lit = {
            vs      = "model_lit_vs.fx.vert",
            fs      = "model_lit_ps.fx.frag",
        },
        BackDistortion = {
            vs      = "model_distortion_vs.fx.vert",
            fs      = "model_distortion_ps.fx.frag",
        },
        AdvancedUnlit = {
            vs      = "ad_model_unlit_vs.fx.vert",
            fs      = "ad_model_unlit_ps.fx.frag",
        },
        AdvancedLit = {
            vs      = "ad_model_lit_vs.fx.vert",
            fs      = "ad_model_lit_ps.fx.frag",
        },
        AdvancedBackDistortion = {
            vs      = "ad_model_distortion_vs.fx.vert",
            fs      = "ad_model_distortion_ps.fx.frag",
        },
    },
}

local shaderc = cwd / bgfxbin_dir / ("shaderc%s.exe"):format(name_suffix)

local sc = require "buildscripts.shader_compile"

local function print_cfg(cfg)
    for k, v in pairs(cfg) do
        print(k, tostring(v))
    end
end

local function cvt2bgfxshader(input, output, shadertype, stage, modeltype)
    lm:build {
        "$luamake", "lua", "efkmatc/genbgfxshader.lua", "$in", "$out", shadertype, stage, modeltype,
        input = input:string(),
        output = output:string(),
    }
end

local files_built = {}

local function build_eff_shader(input, output, defines, stagetype, shadertype, modeltype)
    if files_built[input:string()] then
        return
    end

    local bgfxsrc = fs.path(output):replace_extension "sc"

    files_built[input:string()] = bgfxsrc
    cvt2bgfxshader(input, bgfxsrc, shadertype, stagetype, modeltype)

    local varying_filename = ("%s_%s_varying.def.sc"):format(modeltype, shadertype)
    local varying_path = bgfxsrc:parent_path() / varying_filename
    local cfg = {
        renderer = platform_renderers[lm.os],
        stage = stagetype,
        plat = lm.os,
        optimizelevel = 3,
        debug = true,
        includes = {
            cwd / bgfxdir / "src",
            cwd / bgfx_example_dir / "common",
        },
        defines = defines,
        varying_path = varying_path:string(),
        input = bgfxsrc:string(),
        output = output:string(),
    }

    --print_cfg(cfg)

    local cmd = sc.gen_cmd(shaderc:string(), cfg)
    --print(table.concat(cmd, " "))

    lm:build(cmd)
end

for modeltype, shaders in pairs(shaderfiles) do
    for st, shader in pairs(shaders) do
        for _, stage in ipairs{"vs", "fs"} do
            local filename = shader[stage]
            local infile = vulkan_shader_dir / filename
            local outfile = shader_output_dir / fs.path(filename):replace_extension "bin"
            build_eff_shader(infile, outfile, shader.defines, stage, st, modeltype)
        end
    end
end

----------------------------------------------------------------
local cube_shader_dir = fs.path "examples/cube/shaders"
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
        renderer = platform_renderers[lm.os],
        stage = stage,
        plat = lm.os,
        optimizelevel = 3,
        debug = true,
        includes = {
            cwd / bgfxdir / "src",
            cwd / bgfx_example_dir / "common",
        },
        defines = {},
        input = input:string(),
        output = output:string(),
    }
    local cmd = sc.gen_cmd(shaderc:string(), cfg)
    lm:build(cmd)
end