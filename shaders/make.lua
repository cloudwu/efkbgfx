local lm = require "luamake"
local fs = require "bee.filesystem"

package.path = "./?.lua;../?.lua"

require "buildscripts.common"

lm:import "../efkmatc/make.lua"

local cwd = fs.path(lm.workdir)
local vulkan_shader_dir = (cwd / lm.EfkDir) / "Effekseer/Dev/Cpp/EffekseerRendererVulkan/EffekseerRenderer/Shader"
local shader_output_dir = cwd

local sc = require "buildscripts.shader_compile"

local function print_cfg(cfg)
    for k, v in pairs(cfg) do
        print(k, tostring(v))
    end
end

local function cvt2bgfxshader(input, output, shadertype, stage, modeltype)
    lm:build {
        "$luamake", "lua", "@../efkmatc/genbgfxshader.lua", "$in", "$out", shadertype, stage, modeltype, "@../efkmatc",
        deps = "efkmat",
        description = "Compile sc: $out",
        input = input:string(),
        output = output:string(),
    }
end

local scfiles = {}

local function get_shaders_info(src_shaderpath)
    local ss = {}
    for fn in fs.pairs(src_shaderpath) do
        local ext = fn:extension():string():lower()
        if ext == ".vert" or ext == ".frag" then
            ss[#ss+1] = ShaderInfoFromFilename(fn:string())
        end
    end

    return ss
end

for _, s in ipairs(get_shaders_info(vulkan_shader_dir)) do
    local input = vulkan_shader_dir / s.filename
    local scfile = shader_output_dir / fs.path(s.filename):replace_extension "sc"
    scfiles[#scfiles+1] = scfile
    cvt2bgfxshader(input, scfile, s.shadertype, s.stage, s.modeltype)
end

lm:phony "efxbgfx_shaders" {
    input = scfiles
}