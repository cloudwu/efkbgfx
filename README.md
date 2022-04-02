A bgfx renderer for effekseer runtime
=====

* bgfx : https://github.com/bkaradzic/bgfx
* effekseer : https://github.com/effekseer/Effekseer

Status
======

It's ready to use, and it keeps up with the latest versions of bgfx and effekseer.

All the predefined materials works, but user defined materials are not supported now. It will be supported in future.

The callback APIs may change in future , but it will always be simple enough.

Contributors are welcome.

Build
=====

Use luamake : https://github.com/actboy168/luamake

Or put source files (in renderer) into your project, and link effekseer library.

How to use
==========

You should initialize a structure `EffekseerRendererBGFX::InitArgs` to create a bgfx renderer.

```
EFXBGFX_API EffekseerRenderer::RendererRef CreateRenderer(struct InitArgs *init);
EFXBGFX_API Effekseer::ModelRendererRef CreateModelRenderer(EffekseerRenderer::RendererRef renderer, struct InitArgs *init);
```

These APIs could create the renderer and model renderer for effekseer, See examples for the details.

```C
struct InitArgs {
	int squareMaxCount;	// Max count of sprites
	bgfx_view_id_t viewid;	// The ViewID that effekseer will render to.
	bgfx_interface_vtbl_t *bgfx;	// The bgfx APIs. Use `bgfx_get_interface()`.

	// Some callback functions, See below for details.
	bgfx_shader_handle_t (*shader_load)(const char *mat, const char *name, const char *type, void *ud);
	bgfx_texture_handle_t (*texture_load)(const char *name, int srgb, void *ud);
	bgfx_texture_handle_t (*texture_get)(int texture_type, void *parm, void *ud);	// background or depth (with param)
	void (*texture_unload)(bgfx_texture_handle_t handle, void *ud);
	void * ud;
};
```

```C
bgfx_shader_handle_t shader_load(const char *mat, const char *name, const char *type, void *ud);
```

When renderer need a shader, it will call this function.
* `mat` is the material name, NULL for predefined material.
* `name` is the shader name.
* `type` is "vs" or "fs".
* `ud` is arguments from `InitArgs`.

The predefined shaders for bgfx is at `shaders` dir, compile them with bgfx toolset by yourself.

This function should return a valid bgfx shader handle.

```C
bgfx_texture_handle_t texture_load(const char *name, int srgb, void *ud);
```

When renderer need a texture, it will call this function.
* `name` is the texture name.
* `srgb` true means it should be created as a SRGB texture.
* `ud` is arguments from `InitArgs`.

This function should return a valid bgfx texture handle.

```C
void texture_unload(bgfx_texture_handle_t handle, void *ud);

```
If the texture loaded by `texture_load` has not be no longer used by renderer, this function will be called.


```C
bgfx_texture_handle_t texture_get(int texture_type, void *parm, void *ud);
```
When renderer need the `TEXTURE_BACKGROUND` (usually it's a render target) or the `TEXTURE_DEPTH`, it will call this function.
When the `texture_type` is `TEXTURE_DEPTH`, you should offer an additional argument `DepthReconstructionParameter`.

```C
struct DepthReconstructionParameter {
	float DepthBufferScale;
	float DepthBufferOffset;
	float ProjectionMatrix33;
	float ProjectionMatrix34;
	float ProjectionMatrix43;
	float ProjectionMatrix44;
};
```

This function should return a valid bgfx texture handle.

This callback function can be NULL for optional. If you haven't offer this callback, some features of effekseer will be disabled.
