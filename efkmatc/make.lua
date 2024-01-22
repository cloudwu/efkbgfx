local lm = require "luamake"
package.path = "./?.lua;../?.lua"

require "buildscripts.common"

lm:lua_dll "efkmat" {
    includes = EfkLib_Includes,
    sources = {
        "efkmat.cpp",
    },
    deps = "source_efklib",
    bindir = lm.workdir
}
