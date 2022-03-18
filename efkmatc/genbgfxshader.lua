local efkmat = require "efkmat"

local path = "../Effekseer/Dev/Cpp/EffekseerRendererVulkan/EffekseerRenderer/Shader/"
local output_path = "examples/shaders/"

local filelist = {
	"ad_model_distortion_ps.fx.frag",
	"ad_model_distortion_vs.fx.vert",
	"ad_model_lit_ps.fx.frag",
	"ad_model_lit_vs.fx.vert",
	"ad_model_unlit_ps.fx.frag",
	"ad_model_unlit_vs.fx.vert",
	"ad_sprite_distortion_vs.fx.vert",
	"ad_sprite_lit_vs.fx.vert",
	"ad_sprite_unlit_vs.fx.vert",
	"model_distortion_ps.fx.frag",
	"model_distortion_vs.fx.vert",
	"model_lit_ps.fx.frag",
	"model_lit_vs.fx.vert",
	"model_unlit_ps.fx.frag",
	"model_unlit_vs.fx.vert",
	"sprite_distortion_vs.fx.vert",
	"sprite_lit_vs.fx.vert",
	"sprite_unlit_vs.fx.vert",
}

local ShaderType = {
	Unlit = 0,
	Lit = 1,
	BackDistortion = 2,
	AdvancedUnlit = 3,
	AdvancedLit = 4,
	AdvancedBackDistortion = 5,
}

local function gen(filename)
	local line = { "" }
	for l in io.lines(filename) do
		if l:find "^##" then
			-- skip
		else
			if l:find "[^%s]" then
				table.insert(line, l )
			end
		end
	end

	local source = table.concat(line , "\n")
	local shader = {
		struct = {},
		layout = {},
		uniform = {},
		func = {},
		texture = {},
	}
	local valid_lines = {}
	-- struct name {};
	for name, data in source:gmatch "\n(struct%s+[%w_]+%s*)\n(%b{};)" do
		table.insert(valid_lines, name)
		table.insert(valid_lines, data)

		local struct = { name = name:match "struct%s+([%w_]+)", data = data }
		for k,v in data:gmatch "\n%s+(%w+)%s+([%w_]+);" do
			table.insert(struct, { name = v, type = k })
		end
		table.insert(shader.struct, struct)
	end
	-- layout(...) ... name;
	for layout in source:gmatch "\n(layout[^\n]+;)" do
		table.insert(valid_lines, layout)
		local attrib, type, name = layout:match "layout(%b())%s+(.+)%s+([%w_]+);$"
		local id = tonumber(attrib:match "location%s*=%s*(%d+)")
		if id then
			local inout, t = type:match "(in) (%w+)"
			if inout == nil then
				inout, t = type:match "(out) (%w+)"
			end
			table.insert(shader.layout, {
				name = name,
				inout = inout,
				type = t,
				id = id,
			})
		else
			id = tonumber(attrib:match "binding%s*=%s*(%d+)")
			if id == nil then
				error("Invalid layout", layout)
			end
			assert(type == "uniform sampler2D")
			table.insert(shader.texture, {
				binding = id,
				name = name,
			})
		end
	end
	for layout, data in source:gmatch "\n(layout.-)\n(%b{}%s+[%w_]+;)" do
		table.insert(valid_lines, layout)
		table.insert(valid_lines, data)
		local attrib, type = layout:match "layout(%b())%s+(.+)%s*$"
		local kv, name = data:match "(%b{})%s*([%w_]+);"
		local layout = {
			type = type,
			attrib = attrib,
			name = name,
		}
		for k,v in kv:gmatch "\n%s+(.-)%s+([%w_]+);" do
			k = k:match "[%w_]*$"
			table.insert(layout, { type = k, name = v })
		end
		table.insert(shader.uniform, layout)
	end
	for func, data in source:gmatch "\n(%w+[^\n]-%b())\n(%b{})" do
		table.insert(valid_lines, func)
		table.insert(valid_lines, data)
		local funcname = func:match "[%w_]+ ([%w_]+)%s*%("
		local t = {
			desc = func,
			imp = data,
		}
		table.insert(shader.func, t)
		shader.func[funcname] = t
	end

	local text = table.concat(valid_lines, "\n")

	-- check source
	for id, l in ipairs(line) do
		if l:sub(1,1) ~= "#" then
			if l:find "[^%s]" then
				if not text:find(l, 1, true) then
					error(string.format("%s : (%d) %s", filename, id, l))
				end
			end
		end
	end

	return shader
end

local SemanticNormal = {
	"COLOR0",
	"NORMAL",
	"TANGENT",
	"BITANGENT",
	"COLOR1",
	"COLOR2",
	"COLOR3",
}

local function gen_varying(s, layout)
	local input = {}
	local output = {}
	local input_name = {}
	local input_ps = {}
	local output_name = {}
	local map = {}
	local map_ps = {}
	local isPS = false
	for i, v in ipairs(s.layout) do
		local location = v.id + 1
		if v.inout == "in" then
			local shader_layout = layout[location]
			local type = shader_layout.SemanticName
			if type == "NORMAL" then
				type = assert(SemanticNormal[shader_layout.SemanticIndex + 1])
			elseif type == "TEXCOORD" then
				type = type .. shader_layout.SemanticIndex
			end
			local name = "a_" .. type:lower()
			map[v.name] = name
			input[location] = string.format("%s %s : %s;", v.type, name, type)
			input_name[location] = name
			local psname = "v_" .. v.name:match "Input_(.+)"
			input_ps[location] = psname
			map_ps[v.name] = psname
		else
			assert(v.inout == "out")
			if v.name == "_entryPointOutput" then -- It's ps
				isPS = true
			else
				local name = v.name:gsub("_entryPointOutput_", "v_")
				map[v.name] = name
				output[location] = string.format("%s %s : TEXCOORD%d;", v.type, name, v.id)
				output_name[location] = name
			end
		end
	end

	local varying = {}
	varying.input = table.concat(input, "\n")
	varying.output = table.concat(output, "\n")
	varying.map = map
	if isPS then
		varying.header = string.format("$input %s",	table.concat(input_ps, ","))
		for k,v in pairs(map_ps) do
			map[k] = v
		end
	else
		varying.header = string.format("$input %s\n$output %s",
			table.concat(input_name, ","), table.concat(output_name, ","))
	end

	return varying
end

local function gen_uniform(s)
	local uniform = {}
	local map = {}
	local u = s.uniform[1]
	for i,item in ipairs(u) do
		table.insert(uniform, string.format("uniform %s u_%s;",item.type, item.name))
		map[u.name .. "." .. item.name] = "u_" .. item.name
	end
	return {
		uniform = table.concat(uniform, "\n"),
		map = map,
	}
end

local function gen_texture(s)
	local texture = {}
	local map = {}
	for i, item in ipairs(s.texture) do
		local name = item.name:match "_%w+$"
		table.insert(texture, string.format("SAMPLER2D (s%s,%d);", name, item.binding - 1))
		map["texture("..item.name] = "texture2D(s"..name
	end
	return {
		texture = table.concat(texture, "\n"),
		map = map,
	}
end

local shader_temp=[[
$header

#include <bgfx_shader.sh>

$uniform
$texture

$struct

$source
]]

local function genshader(name, type)
	local fullname = path .. name
	local s = gen(fullname)
	local varying = gen_varying(s, efkmat.layout(ShaderType[type]))
	local uniform = gen_uniform(s)
	local texture = gen_texture(s)

	local func = s.func
	local main = func.main.imp:gsub("[%w_]+", varying.map)
	main = main:gsub("_entryPointOutput", "gl_FragColor")
	func.main.imp = main

	func._main.imp = func._main.imp:gsub("[%w_]+", uniform.map)

	for i, f in ipairs(func) do
		f.imp = f.imp:gsub("texture%([%w_]+", texture.map)
	end

	local source = {}
	for i, v in ipairs(s.func) do
		source[i] = v.desc .. "\n" .. v.imp
	end
	local struct = {}
	for i,v in ipairs(s.struct) do
		struct[i] = string.format("struct %s\n%s", v.name, v.data)
	end

	return {
		source = shader_temp:gsub("$(%l+)", {
			header = varying.header,
			uniform = uniform.uniform,
			texture = texture.texture,
			struct = table.concat(struct, "\n\n"),
			source = table.concat(source, "\n\n"),
		}),
		varying = varying.input .. "\n" .. varying.output,
	}
end

local function writefile(filename, text)
	local f = assert(io.open(output_path .. filename, "wb"))
	f:write(text)
	f:close()
end

local r = genshader("sprite_lit_vs.fx.vert", "Lit")
writefile("lit_varying.def.sc", r.varying)
writefile("vs_sprite_lit.sc", r.source)

local r = genshader("model_lit_ps.fx.frag", "Lit")
writefile("fs_model_lit.sc", r.source)
