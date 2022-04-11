local lm = require "luamake"

package.path = "./?.lua;../?.lua"

require "buildscripts.common"

lm.builddir = lm.builddir or ("build/%s/%s"):format(Plat, lm.mode)
lm.bindir   = lm.bindir or ("bin/%s/%s"):format(Plat, lm.mode)

lm:source_set "source_efklib" {
    includes = EfkLib_Includes,
    sources = ToStrings{
        EfkSrc / "Effekseer/Effekseer/**.cpp",
        EfkSrc / "EffekseerMaterial/*.cpp",
        EfkSrc / "EffekseerRendererCommon/*.cpp",
    },
    defines = {
        lm.mode == "debug" and "_DEBUG_EFFEKSEER=1" or nil
    }
}

lm:lib "efklib" {
    deps = "source_efklib",
}

MaxInstanced = 20

<<<<<<< HEAD
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
        }
=======
lm:source_set "source_efkbgfx" {
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
>>>>>>> e88d7517c23242cfb4fa4f625bb12649c231b1ae
    }
end

lm:source_set "source_efkbgfx_lib" (create_source_efkbgfx())
lm:source_set "source_efkbgfx" (
    create_source_efkbgfx{
        "EFXBGFX_DYNAMIC_LIB=1",
        "EFXBGFX_EXPORTS=1",
    })

lm:lib "efkbgfx_lib" {
    deps = {"source_efkbgfx_lib"}
}

lm:dll "efkbgfx" {
<<<<<<< HEAD
=======
    defines = {
        "EFXBGFX_DYNAMIC_LIB=1",
        "EFXBGFX_EXPORTS=1",
    },
>>>>>>> e88d7517c23242cfb4fa4f625bb12649c231b1ae
    deps = {"source_efkbgfx"},
}