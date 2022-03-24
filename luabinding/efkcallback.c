#define LUA_LIB

#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <string.h>

typedef void (*voidf)(void);

struct handle_t {
	uint16_t idx;
};

static struct handle_t invalid_handle = { 0xffff };

#define CALLBACK_SHADER_LOAD 1
#define CALLBACK_TEXTURE_LOAD 2
#define CALLBACK_TEXTURE_UNLOAD 3
#define CALLBACK_ERROR_HANDLER 4
#define CALLBACK_TOP 4

#define TEXTURE_BACKGROUND 0
#define TEXTURE_DEPTH 1

struct callback_ud {
	lua_State *L;
	int error_handler;
	struct handle_t background;
	struct handle_t depth;
	float depth_param[6];
};

static void
set_callback(lua_State *L, struct callback_ud *ud, const char *name, int id) {
	if (lua_getfield(L, 1, name) != LUA_TFUNCTION) {
		luaL_error(L, "%s is not a function");
	}
	lua_xmove(L, ud->L, 1);
	lua_replace(ud->L, id);
}

static void
set_depth(lua_State *L, struct callback_ud *ud) {
	if (lua_getfield(L, 3, "handle") != LUA_TNUMBER) {
		luaL_error(L, "Missing depth.handle");
	}
	if (!lua_isinteger(L, -1)) {
		luaL_error(L, "Invalid depth.handle");
	}
	ud->depth.idx = lua_tointeger(L, -1) & 0xffff;
	lua_pop(L, 1);
	int i;
	for (i=0;i<6;i++) {
		if (lua_geti(L, 3, i+1) != LUA_TNUMBER) {
			luaL_error(L, "Missing depth params at %d", i+1);
		}
		ud->depth_param[i] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
}

static int
lset_texture(lua_State *L) {
	struct callback_ud *ud = (struct callback_ud *)lua_touserdata(L, 1);
	const char *name = luaL_checkstring(L, 2);
	if (strcmp(name, "background")==0) {
		uint16_t handle = luaL_checkinteger(L, 3) & 0xffff;
		ud->background.idx = handle;
	} else if (strcmp(name, "depth")) {
		luaL_checktype(L, 3, LUA_TTABLE);
		set_depth(L, ud);
	} else {
		return luaL_error(L, "Invalid attrib %s", name);
	}
	return 0;
}

static int
lcallback(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	struct callback_ud *ud = (struct callback_ud *)lua_newuserdatauv(L, sizeof(*ud), 1);
	ud->L = lua_newthread(L);
	ud->error_handler = 0;
	lua_setiuservalue(L, -2, 1);
	lua_settop(ud->L, CALLBACK_TOP);
	set_callback(L, ud, "shader_load", CALLBACK_SHADER_LOAD);
	set_callback(L, ud, "texture_load", CALLBACK_TEXTURE_LOAD);
	set_callback(L, ud, "texture_unload", CALLBACK_TEXTURE_UNLOAD);
	if (lua_getfield(L, 1, "error") == LUA_TFUNCTION) {
		lua_xmove(L, ud->L, 1);
		lua_replace(ud->L, CALLBACK_ERROR_HANDLER);
		ud->error_handler = CALLBACK_ERROR_HANDLER;
	}

	lua_newtable(L);
	lua_pushcfunction(L, lset_texture);
	lua_setfield(L, -2, "__newindex");
	lua_setmetatable(L, -2);

	return 1;
}

static struct handle_t
ret_handle(struct callback_ud *ud, int args) {
	if (lua_pcall(ud->L, args, 1, ud->error_handler) != LUA_OK || !lua_isinteger(ud->L, -1)) {
		lua_pop(ud->L, 1);
		return invalid_handle;
	}
	struct handle_t ret = { lua_tointeger(ud->L, -1) & 0xffff };
	lua_pop(ud->L, 1);
	return ret;
}

static struct handle_t
shader_load(const char *mat, const char *name, const char *type, struct callback_ud *ud) {
	lua_State *L = ud->L;
	lua_pushvalue(L, CALLBACK_SHADER_LOAD);
	if (mat == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushstring(L, mat);
	}
	lua_pushstring(L, name);
	lua_pushstring(L, type);
	return ret_handle(ud, 3);
}

static struct handle_t
texture_load(const char *name, int srgb, struct callback_ud *ud) {
	lua_State *L = ud->L;
	lua_pushvalue(L, CALLBACK_TEXTURE_LOAD);
	lua_pushstring(L, name);
	lua_pushboolean(L, srgb);
	return ret_handle(ud, 2);
}

static void
texture_unload(struct handle_t handle, struct callback_ud *ud) {
	lua_State *L = ud->L;
	lua_pushvalue(L, CALLBACK_TEXTURE_UNLOAD);
	lua_pushinteger(L, handle.idx);
	if (lua_pcall(L, 2, 0, ud->error_handler) != LUA_OK) {
		lua_pop(L, 1);
	}
}

static struct handle_t
texture_get(int texture_type, void *param, struct callback_ud *ud) {
	switch (texture_type) {
	case TEXTURE_BACKGROUND:
		return ud->background;
	case TEXTURE_DEPTH:
		memcpy(param, ud->depth_param, sizeof(ud->depth_param));
		return ud->depth;
	}
	return invalid_handle;
}

static void
set_capi(lua_State *L, const char *name, voidf f) {
	lua_pushlightuserdata(L, (void *)f);
	lua_setfield(L, -2, name);
}

LUAMOD_API int
luaopen_effekseer_callback(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "callback", lcallback },
		{ "shader_load", NULL },
		{ "texture_load", NULL },
		{ "texture_get", NULL },
		{ "texture_unload", NULL },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	set_capi(L, "shader_load", (voidf)shader_load);
	set_capi(L, "texture_load", (voidf)texture_load);
	set_capi(L, "texture_get", (voidf)texture_get);
	set_capi(L, "texture_unload", (voidf)texture_unload);
	return 1;
}