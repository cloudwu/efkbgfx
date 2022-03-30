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

local genbgfxshader_path = lm.workdir .. "/../efkmatc"
genbgfxshader_path = genbgfxshader_path:gsub(fs.current_path():string() .. "/", "")
local genbgfxshader_file = genbgfxshader_path .. "/genbgfxshader.lua"
local function cvt2bgfxshader(input, output, shadertype, stage, modeltype)
    lm:build {
        "$luamake", "lua", genbgfxshader_file, genbgfxshader_path, "$in", "$out", shadertype, stage, modeltype,
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
        stage = stagetype,
        optimizelevel = 3,
        debug = true,
        includes = {
            cwd / BgfxDir / "src",
        },
        defines = defines,
        varying_path = varying_path:string(),
        input = bgfxsrc:string(),
        output = output:string(),
    }

    --print_cfg(cfg)

    local cmd = sc.gen_cmd(Shaderc:string(), cfg)
    --print(table.concat(cmd, " "))

    lm:build(cmd)
end

local outshaders = {}
for modeltype, shaders in pairs(shaderfiles) do
    for st, shader in pairs(shaders) do
        for _, stage in ipairs{"vs", "fs"} do
            local filename = shader[stage]
            local infile = vulkan_shader_dir / filename
            local outfile = shader_output_dir / fs.path(filename):replace_extension "bin"
            build_eff_shader(infile, outfile, shader.defines, stage, st, modeltype)
            outshaders[#outshaders+1] = outfile:string()
        end
    end
end

lm:phony "efkbgfx_shaders" {
    deps = {
        "efkmat"
    },
    input = outshaders,
}