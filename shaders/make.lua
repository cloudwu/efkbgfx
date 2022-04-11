local lm = require "luamake"
local fs = require "bee.filesystem"

package.path = "./?.lua;../?.lua"

require "buildscripts.common"

lm:import "../efkmatc/make.lua"

local cwd = fs.path(lm.workdir)
local vulkan_shader_dir = (cwd / lm.EfkDir) / "Effekseer/Dev/Cpp/EffekseerRendererVulkan/EffekseerRenderer/Shader"
local shader_output_dir = cwd

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

local sc = require "buildscripts.shader_compile"

local function print_cfg(cfg)
    for k, v in pairs(cfg) do
        print(k, tostring(v))
    end
end

local function cvt2bgfxshader(input, output, shadertype, stage, modeltype)
    lm:build {
        "cmd", "/c",
        "cd", lm.workdir .. "/../efkmatc", "&&",
        "$luamake", "lua", "../efkmatc/genbgfxshader.lua", "$in", "$out", shadertype, stage, modeltype,
        deps = {
            "efkmat"
        },
        input = input:string(),
        output = output:string(),
    }
end

local files_built = {}

local function build_eff_shader(input, scfile, output, defines, stagetype, shadertype, modeltype)
    if files_built[input:string()] then
        return
    end

    files_built[input:string()] = scfile
    cvt2bgfxshader(input, scfile, shadertype, stagetype, modeltype)

    local varying_filename = ("%s_%s_varying.def.sc"):format(modeltype, shadertype)
    local varying_path = scfile:parent_path() / varying_filename
    local cfg = {
        stage = stagetype,
        optimizelevel = 3,
        debug = true,
        includes = {
            cwd / BgfxDir / "src",
        },
        defines = defines,
        varying_path = varying_path:string(),
        input = scfile:string(),
        output = output:string(),
    }

    --print_cfg(cfg)
    local cmd = sc.gen_cmd(Shaderc:string(), cfg)
    --print(table.concat(cmd, " "))
    lm:build(cmd)
end

local shader_target_files = {
    inputs = {},
    scfiles = {},
    outputs = {},
}

for modeltype, shaders in pairs(shaderfiles) do
    for st, shader in pairs(shaders) do
        for _, stage in ipairs{"vs", "fs"} do
            local filename = shader[stage]
            local input = vulkan_shader_dir / filename
            local scfile = shader_output_dir / fs.path(filename):replace_extension "sc"
            local output = fs.path(scfile):replace_extension "bin"
            shader_target_files.inputs[#shader_target_files.inputs+1]   = input:string()
            shader_target_files.scfiles[#shader_target_files.scfiles+1] = scfile:string()
            shader_target_files.outputs[#shader_target_files.outputs+1] = output:string()
            build_eff_shader(input, scfile, output, shader.defines, stage, st, modeltype)
        end
    end
end

lm:phony "efxbgfx_shaders" {
    input = shader_target_files.scfiles,
}

lm:phony "efkbgfx_shader_binaries" {
    input = shader_target_files.outputs,
}