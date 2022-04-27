local lm = require "luamake"

lm:import "../renderer/make.lua"
lm:import "../shaders/make.lua"

if lm.LuaInclude == nil then
    error "need to define lua directory"
end

lm:source_set "source_effekseer_callback" {
    includes = {
        lm.LuaInclude
    },
    sources = {
        "efkcallback.c",
    },
}

lm:lua_dll "effekseer_callback" {
    deps = {
        "source_efkbgfx_lib",
        "source_effekseer_callback",
    }
}