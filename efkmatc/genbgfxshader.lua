local modulepath	= arg[1]
package.cpath = table.concat({
	"efkmatc/?.dll",
	"./?.dll",
	modulepath .. "/?.dll"
}, ";")

print("cpath", package.cpath)

local input 		= arg[2]
local output 		= arg[3]
local shadertype 	= arg[4]
local stage 		= arg[5]
local modeltype 	= arg[6]

local efkmat = require "efkmat"

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
	-- uniform
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
		for k,v in kv:gmatch "\n%s+(.-)%s+([%w_%[%]]+);" do
			k = k:match "[%w_]*$"
			local name, array = v:match "(.+)%[(%d+)%]"
			if name then
				array = tonumber(array)
			else
				name = v
			end
			table.insert(layout, { type = k, name = name, array = array })
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

local SPRITE_SEMANTIC<const> = {
	POSITION0 	= {"a_position", 	"POSITION"},
	TEXCOORD0 	= {"a_texcoord0", 	"TEXCOORD0"},
	NORMAL0 	= {"a_color0", 		"COLOR0"},
	NORMAL1 	= {"a_normal",		"NORMAL"},
	NORMAL2 	= {"a_tangent", 	"TANGENT"},
	TEXCOORD1	= {"a_texcoord1",	"TEXCOORD1"},
	TEXCOORD2	= {"a_texcoord2",	"TEXCOORD2"},
	TEXCOORD3	= {"a_texcoord3",	"TEXCOORD3"},
	TEXCOORD4	= {"a_texcoord4",	"TEXCOORD4"},
	TEXCOORD5	= {"a_texcoord5",	"TEXCOORD5"},
	TEXCOORD6	= {"a_texcoord6",	"TEXCOORD6"},
}

local MODEL_SEMANTIC<const> = {
	POSITION0 	= {"a_position", 	"POSITION"},
	TEXCOORD0 	= {"a_texcoord0", 	"TEXCOORD0"},
	NORMAL0 	= {"a_normal", 		"NORMAL"},
	NORMAL1 	= {"a_bitangent",	"BITANGENT"},
	NORMAL2 	= {"a_tangent", 	"TANGENT"},
	NORMAL3 	= {"a_color0", 		"COLOR0"},
}

local VAYRING_pat = "%s %s : %s;"

local function load_vs_varying(s, shadertype, modeltype)
	local input, output = {}, {}
	local input_names, output_names = {}, {}
	local map = {}
	local layout = modeltype == "model" and efkmat.model_layout() or efkmat.layout(ShaderType[shadertype])
	local mapper = modeltype == "model" and MODEL_SEMANTIC or SPRITE_SEMANTIC

	for i, v in ipairs(s.layout) do
		local location = v.id + 1
		if v.inout == "in" then
			local shader_layout = layout[location]
			local m = mapper[shader_layout.SemanticName .. shader_layout.SemanticIndex]
			local name, type = m[1], m[2]
			map[v.name] = name
			input[location] = VAYRING_pat:format(v.type, name, type)
			input_names[location] = name
		else
			assert(v.inout == "out")
			local name = v.name:gsub("_entryPointOutput_", "v_")
			map[v.name] = name
			output[location] = VAYRING_pat:format(v.type, name, "TEXCOORD" .. v.id)
			output_names[location] = name
		end
	end

	return {
		file	= table.concat(input, "\n") .. "\n" .. table.concat(output, "\n"),
		map		= map,
		header 	= ("$input %s\n$output %s"):format(
				table.concat(input_names, " "), table.concat(output_names, " "))
	}
end

local function load_fs_varying(s)
	local input_names = {}
	local map = {}
	for i, v in ipairs(s.layout) do
		local location = v.id + 1
		if v.inout == "in" then
			local sn = v.name:match "Input_([%w_]+)"
			local name = "v_" .. sn
			map[v.name] = name
			input_names[location] = name
		end
	end

	return {
		map = map,
		header = "$input " .. table.concat(input_names, " ")
	}
end

local function gen_varying(s, stagetype, shadertype, modeltype)
	if stagetype == "vs" then
		return load_vs_varying(s, shadertype, modeltype)
	end

	return load_fs_varying(s)
end

local function gen_uniform(s)
	local uniform = {}
	local map = {}
	local u = s.uniform[1]
	for i,item in ipairs(u) do
		if item.array then
			table.insert(uniform, string.format("uniform %s u_%s[%d];",item.type, item.name, item.array))
		else
			table.insert(uniform, string.format("uniform %s u_%s;",item.type, item.name))
		end
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
		item.new_name = "s" .. name
		map["texture("..item.name] = "texture2D("..item.new_name
	end
	return {
		texture = table.concat(texture, "\n"),
		map = map,
	}
end

local shader_temp=[[
$header

#include <bgfx_shader.sh>
#include "defines.sh"
$uniform
$texture

$struct

$source
]]

local function genshader(fullname, stagetype, type, modeltype)
	local s = gen(fullname)
	local varying = gen_varying(s, stagetype, type, modeltype)
	local uniform = gen_uniform(s)
	local texture = gen_texture(s)

	local func = s.func
	local main = func.main.imp:gsub("[%w_]+", varying.map)
	main = main:gsub("_entryPointOutput", "gl_FragColor")
	main = main:gsub("\n%s*_position.y%s*=%s*-_position.y;", "")
	func.main.imp = main

	for i, f in ipairs(func) do
		-- uniform
		f.imp = f.imp:gsub("[%w_]+", uniform.map)
		f.imp = f.imp:gsub("[%w_]+%.[%w_]+", uniform.map)

		--texture
		f.desc = f.desc:gsub("sampler2D", "struct BgfxSampler2D")
		f.imp = f.imp:gsub("texture%([%w_]+", function(m)
			local n = texture.map[m]
			if n then
				return n
			end

			local tn = m:match "%(([%w_]+)"
			if f.desc:match(tn) == nil then
				error(("texture name not match:%s"):format(m))
			end
			return "texture2D(" .. tn
		end)

		for _, t in ipairs(s.texture) do
			f.imp = f.imp:gsub(t.name, t.new_name)
		end

		--shader dependent code
		do
			local tangentTransFmt = ("mat3(vec3(Input.WorldT),vec3(Input.WorldB),vec3(Input.WorldN))"):gsub("%(", "%%s*%%(")
			tangentTransFmt = tangentTransFmt:gsub("%)", "%%s*%%)")
			tangentTransFmt = tangentTransFmt:gsub(",", "%%s*,%%s*")
			tangentTransFmt = tangentTransFmt .. "%s*%*%s*([%w+_]+)"
			f.imp = f.imp:gsub(tangentTransFmt, "mul(mtxFromCols(Input.WorldT, Input.WorldB, Input.WorldN), %1)")
		end

		do
			local vec_pat = "vec([432])(%b())"
			local vec_handler
			vec_handler = function (m1, m2)
				m2 = m2:gsub(vec_pat, vec_handler)
				local pat = "vec%s_splat%s"
				-- for number, like: vec3(0.5e-7) or vec3(pow(x, 0.5))
				if m2:match "^%([-%w_.]+%)$" or m2:match "^%([-%w_.]+%b()%)$" then
					return pat:format(m1, m2)
				end

				--check no function call and parameter more than 1
				if (not ((m2:match "[%w_]+%b()%s*,") or m2:match ",%s*[%w_]+%b()")) and (not m2:find ",") then
					return pat:format(m1, m2)
				end
				return ("vec%s%s"):format(m1, m2)
			end

			f.imp = f.imp:gsub(vec_pat, vec_handler)
		end
		
		f.imp = f.imp:gsub("(VS_Output%s+Output%s+=)%s*[^;]+;", "%1 (VS_Output)0;")

		do
			local pat1 = "([%w_.]+)"
			local pat2 = "(%b())"
			local mul = "%s*%*%s*"

			local function matrixmul(rv)
				local pats = {
					pat1 .. mul .. rv,
					pat2 .. mul .. rv,
				}

				for _, pat in ipairs(pats) do
					f.imp = f.imp:gsub(pat, ("mul(%s, %%1)"):format(rv))
				end
			end

			matrixmul "u_mCameraProj"
			matrixmul "u_mCamera"
			matrixmul "mModel"
		end
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
		varying = varying.file,
	}
end

local function writefile(filename, text)
	local f = assert(io.open(filename, "wb"))
	f:write(text)
	f:close()
end

local r = genshader(input, stage, shadertype, modeltype)
if r.varying then
	local ppath = output:match "(.+[/\\]).+$"
	local outputVaryingFile = ("%s/%s_%s_varying.def.sc"):format(ppath, modeltype, shadertype)
	writefile(outputVaryingFile, r.varying)
end

writefile(output, r.source)

