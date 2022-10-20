local lm = require "luamake"

lm:import "../renderer/make.lua"

lm:lua_source "source_effekseer_callback" {
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