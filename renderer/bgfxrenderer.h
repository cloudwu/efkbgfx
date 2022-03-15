#ifndef effekseer_bgfx_renderer_h
#define effekseer_bgfx_renderer_h

#include <bgfx/c99/bgfx.h>
#include <Effekseer/Effekseer.h>
#include <EffekseerRendererCommon/EffekseerRenderer.Renderer.h>

namespace EffekseerRendererBGFX {
	struct InitArgs {
		int squareMaxCount;
		bgfx_view_id_t viewid;
		bgfx_interface_vtbl_t *bgfx;
		bgfx_shader_handle_t (*shader_load)(const char *mat, const char *name, const char *type, void *ud);
		bgfx_texture_handle_t (*texture_load)(const char *name, int srgb, void *ud);
		void (*texture_unload)(bgfx_texture_handle_t handle, void *ud);
		void * ud;
	};

	EffekseerRenderer::RendererRef CreateRenderer(struct InitArgs *init);
}

#endif
