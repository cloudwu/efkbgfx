local lm = require "luamake"

package.path = "./?.lua;../?.lua"

require "buildscripts.common"

lm.builddir = lm.builddir or ("build/%s/%s"):format(Plat, lm.mode)
lm.bindir   = lm.bindir or ("bin/%s/%s"):format(Plat, lm.mode)

print("efkbgfx:", lm.builddir, lm.bindir)

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
    }
}

lm:lib "efkbgfx_lib" {
    deps = {"source_efkbgfx"}
}

lm:dll "efkbgfx" {
    defines = {
        "EFXBGFX_DYNAMIC_LIB=1",
        "EFXBGFX_EXPORTS=1",
    },
    deps = {"source_efkbgfx"},
}