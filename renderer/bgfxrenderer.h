#ifndef effekseer_bgfx_renderer_h
#define effekseer_bgfx_renderer_h

#include <bgfx/c99/bgfx.h>
#include <Effekseer/Effekseer.h>
#include <EffekseerRendererCommon/EffekseerRenderer.Renderer.h>

#ifdef _MSC_VER
#	if EFXBGFX_EXPORTS 
#		define EFXBGFX_API __declspec(dllexport)
#	else
#		define EFXBGFX_API __declspec(dllimport)
#	endif
#else //!_MSC_VER
#	define EFXBGFX_API
#endif //_MSC_VER

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

	EFXBGFX_API EffekseerRenderer::RendererRef CreateRenderer(struct InitArgs *init);
	EFXBGFX_API Effekseer::ModelRendererRef CreateModelRenderer(EffekseerRenderer::RendererRef renderer, struct InitArgs *init);
}

#endif
