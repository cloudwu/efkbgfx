local lm = require "luamake"

lm.EfkDir = "../../"
lm.BgfxBinDir = "../bgfx/.build/win64_vs2022/bin"
lm:import "shaders/make.lua"
lm:import "renderer/make.lua"
