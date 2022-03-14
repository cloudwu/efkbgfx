#ifndef effekseer_bgfx_renderer_h
#define effekseer_bgfx_renderer_h

#include <bgfx/c99/bgfx.h>
#include <Effekseer/Effekseer.h>

#if EFXBGFX_EXPORTS 
#define EFXBGFX_API __declspec(dllexport)
#else
#define EFXBGFX_API __declspec(dllimport)
#endif

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

	class Renderer;
	using RendererRef = Effekseer::RefPtr<Renderer>;

	class EFXBGFX_API Renderer : public EffekseerRenderer::Renderer {
	protected:
		Renderer() = default;
		virtual ~Renderer() = default;
	public:
		static RendererRef Create(struct InitArgs *init);
		virtual int32_t GetSquareMaxCount() const = 0;
		virtual void SetSquareMaxCount(int32_t count) = 0;
	};
}

#endif
