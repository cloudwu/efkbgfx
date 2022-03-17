#define LUA_LIB

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <Effekseer/Material/Effekseer.MaterialFile.h>

static int
lloadMat(lua_State *L) {
	size_t sz;
	const char *data = luaL_checklstring(L, 1, &sz);

	Effekseer::MaterialFile mat;
	if (!mat.Load((const uint8_t *)data, (int32_t)sz))
		return luaL_error(L, "Invalid effekseer matrtial");

	lua_newtable(L);
	switch (mat.GetShadingModel()) {
	case Effekseer::ShadingModelType::Lit :
		lua_pushstring(L, "Lit");
		break;
	case Effekseer::ShadingModelType::Unlit :
		lua_pushstring(L, "Unlit");
		break;
	default:
		lua_pushstring(L, "Unknown");
		break;
	}
	lua_setfield(L, -2, "ShadingModel");
	lua_pushboolean(L, mat.GetIsSimpleVertex());
	lua_setfield(L, -2, "IsSimpleVertex");
	lua_pushboolean(L, mat.GetHasRefraction());
	lua_setfield(L, -2, "HasRefraction");
	lua_pushstring(L, mat.GetGenericCode());
	lua_setfield(L, -2, "Code");
	lua_pushinteger(L, mat.GetGUID());
	lua_setfield(L, -2, "GUID");
	lua_pushinteger(L, mat.GetCustomData1Count());
	lua_setfield(L, -2, "CustomData1Count");
	lua_pushinteger(L, mat.GetCustomData2Count());
	lua_setfield(L, -2, "CustomData2Count");

	int i;
	int texture_n = mat.GetTextureCount();
	lua_createtable(L, texture_n, 0);
	for (i=0;i<texture_n;i++) {
		lua_createtable(L, 0, 4);
		switch (mat.GetTextureColorType(i)) {
		case Effekseer::TextureColorType::Color :
			lua_pushstring(L, "Color");
			break;
		case Effekseer::TextureColorType::Value :
			lua_pushstring(L, "Value");
			break;
		default:
			lua_pushstring(L, "Unknown");
			break;
		}
		lua_setfield(L, -2, "ColorType");
		switch (mat.GetTextureWrap(i)) {
		case Effekseer::TextureWrapType::Repeat :
			lua_pushstring(L, "Repeat");
			break;
		case Effekseer::TextureWrapType::Clamp :
			lua_pushstring(L, "Clamp");
			break;
		default:
			lua_pushstring(L, "Unknown");
			break;
		}
		lua_setfield(L, -2, "Wrap");
		lua_pushinteger(L, mat.GetTextureIndex(i));
		lua_setfield(L, -2, "Index");
		lua_pushstring(L, mat.GetTextureName(i));
		lua_setfield(L, -2, "Name");

		lua_rawseti(L, -2, i+1);
	}
	lua_setfield(L, -2, "Texture");

	int uniform_n = mat.GetUniformCount();
	lua_createtable(L, uniform_n, 0);
	for (i=0;i<uniform_n;i++) {
		lua_createtable(L, 0, 2);
		lua_pushinteger(L, mat.GetUniformIndex(i));
		lua_setfield(L, -2, "Index");
		lua_pushstring(L, mat.GetUniformName(i));
		lua_setfield(L, -2, "Name");
		lua_rawseti(L, -2, i+1);
	}
	lua_setfield(L, -2, "Uniform");
	return 1;
}

extern "C" {

LUAMOD_API int
luaopen_efkmat(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "load", lloadMat },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

}