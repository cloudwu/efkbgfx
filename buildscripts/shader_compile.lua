local fs = require "bee.filesystem"
local stage_types = {
	fs = "fragment",
	vs = "vertex",
	cs = "compute",
}

local shader_options = {
	direct3d9 = {
		vs = "vs_3_0",
		fs = "ps_3_0",
	},
	direct3d11 = {
		vs = "vs_5_0",
		fs = "ps_5_0",
		cs = "cs_5_0",
	},
	direct3d12 = {
		vs = "vs_5_0",
		fs = "ps_5_0",
		cs = "cs_5_0",
	},
	opengl = {
		vs = "120",
		fs = "120",
		cs = "430",
	},
	metal = {
		vs = "metal",
		fs = "metal",
		cs = "metal",
	},
	vulkan = {
		vs = "spirv",
		fs = "spirv",
	},
}


local function generate_command(shaderc, cfg)
    local renderer = cfg.renderer
    local stagename = cfg.stage
    local commands = {
        shaderc,
        "-f", "$in", "-o", "$out",
        "--platform", cfg.plat,
        "--type", stage_types[stagename],
        "-p", shader_options[renderer][stagename],
        input = cfg.input,
        output = cfg.output,
    }

    if cfg.varying_path then
        commands[#commands+1] = "--varyingdef"
        commands[#commands+1] = cfg.varying_path
    end

    if cfg.includes then
        for _, p in ipairs(cfg.includes) do
            if not fs.exists(p) then
                error(string.format("include path : %s, but not exist!", p))
            end
            assert(p:is_absolute(), p:string())
            commands[#commands+1] = "-i"
            commands[#commands+1] = p:string()
        end
    end
    
    if cfg.defines then
        local t = {}
        for _, m in ipairs(cfg.defines) do
            t[#t+1] = m
        end
        if #t > 0 then
            local defines = table.concat(t, ';')
            commands[#commands+1] = "--define"
            commands[#commands+1] = defines
        end
    end
    
    local level = cfg.optimizelevel
    if not level then
        if cfg.renderer:match("direct3d") then
            level = cfg.stage == "cs" and 1 or 3
        end
    end
    
    if cfg.debug then
        commands[#commands+1] = "--debug"
    else
        if level then
            commands[#commands+1] = "-O"
            commands[#commands+1] = tostring(level)
        end
    end

    return commands
end

return {
    gen_cmd = generate_command
}