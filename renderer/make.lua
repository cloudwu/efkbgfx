local lm = require "luamake"

package.path = "./?.lua;../?.lua"

require "buildscripts.common"

lm.builddir = lm.builddir or ("build/%s/%s"):format(Plat, lm.mode)
lm.bindir   = lm.bindir or ("bin/%s/%s"):format(Plat, lm.mode)

local gcc_warnings = {
	"-Wno-sign-compare",
	"-Wno-unused-but-set-variable",
	"-Wno-format",
	"-Wno-unused-variable",
}

lm:source_set "source_efklib" {
    includes = EfkLib_Includes,
    sources = ToStrings{
        EfkSrc / "Effekseer/Effekseer/**/*.cpp",
        EfkSrc / "EffekseerMaterial/*.cpp",
        EfkSrc / "EffekseerRendererCommon/*.cpp",
    },
    gcc = {
        flags = gcc_warnings
    }
}

lm:lib "efklib" {
    deps = "source_efklib",
}

MaxInstanced = 20

local function create_source_efkbgfx(defines)
    return  {
        includes = {
            EfkLib_Includes,
        },
        sources = {
            "bgfxrenderer.cpp",
        },
        deps = {
            "source_efklib"
        },
        defines = {
            "BX_CONFIG_DEBUG=" .. (lm.mode == "debug" and 1 or 0),
            "MaxInstanced=" .. MaxInstanced,
            defines,
        },
		gcc = {
		    flags = gcc_warnings
		}
    }
end

lm:source_set "source_efkbgfx_lib" (
    create_source_efkbgfx()
)

lm:source_set "source_efkbgfx" (
    create_source_efkbgfx{
        "EFXBGFX_DYNAMIC_LIB=1",
        "EFXBGFX_EXPORTS=1",
    }
)

lm:dll "efkbgfx" {
    deps = {"source_efkbgfx"},
}
