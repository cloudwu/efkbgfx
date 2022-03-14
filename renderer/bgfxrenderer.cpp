#include <cstdint>
#include <cassert>
#include <cstring>
#include <EffekseerRendererCommon/EffekseerRenderer.IndexBufferBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.VertexBufferBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.ShaderBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.RenderStateBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.Renderer_Impl.h>
#include <EffekseerRendererCommon/EffekseerRenderer.RibbonRendererBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.RingRendererBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.SpriteRendererBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.TrackRendererBase.h>
#include "bgfxrenderer.h"

#define BGFX(api) m_bgfx->api

#define MAX_PATH 2048

namespace EffekseerRendererBGFX {

static const int SHADERCOUNT = (int)EffekseerRenderer::RendererShaderType::Material;

// VertexBuffer for Renderer
class TransientVertexBuffer : public EffekseerRenderer::VertexBufferBase {
private:
	bgfx_transient_vertex_buffer_t m_buffer;
public:
	TransientVertexBuffer(int size) : VertexBufferBase(size, true) {}
	virtual ~TransientVertexBuffer() override = default;
	bgfx_transient_vertex_buffer_t * GetInterface() {
		return &m_buffer;
	}
	bool RingBufferLock(int32_t size, int32_t& offset, void*& data, int32_t alignment) override {
		assert(!m_isLock);
		m_isLock = true;
		m_offset = size;
		data = m_buffer.data;
		offset = 0;
		(void)alignment;
		return true;
	}
	bool TryRingBufferLock(int32_t size, int32_t& offset, void*& data, int32_t alignment) override {
		// Never used
		return RingBufferLock(size, offset, data, alignment);
	}
	void Lock() override {
		assert(!m_isLock);
		m_isLock = true;
		m_offset = 0;
		m_resource = m_buffer.data;
	}
	void Unlock() override {
		assert(m_isLock);
		m_offset = 0;
		m_resource = nullptr;
		m_isLock = false;
	}
};

// Renderer

class VertexLayout;
using VertexLayoutRef = Effekseer::RefPtr<VertexLayout>;

class VertexLayout : public Effekseer::Backend::VertexLayout {
private:
	Effekseer::CustomVector<Effekseer::Backend::VertexLayoutElement> elements_;
public:
	VertexLayout(const Effekseer::Backend::VertexLayoutElement* elements, int32_t elementCount) {
		elements_.resize(elementCount);
		for (int32_t i = 0; i < elementCount; i++) {
			elements_[i] = elements[i];
		}
	}
	~VertexLayout() = default;
	const Effekseer::CustomVector<Effekseer::Backend::VertexLayoutElement>& GetElements() const	{
		return elements_;
	}
};

class RendererImplemented;
using RendererImplementedRef = ::Effekseer::RefPtr<RendererImplemented>;

class Texture : public Effekseer::Backend::Texture {
private:
	const RendererImplemented *m_render;
	bgfx_texture_handle_t m_handle;
public:
	Texture(const RendererImplemented *render, bgfx_texture_handle_t handle) : m_render(render), m_handle(handle) {};
	~Texture() override;
	bgfx_texture_handle_t GetInterface() const {
		return m_handle;
	}
	bgfx_texture_handle_t RemoveInterface() {
		bgfx_texture_handle_t ret = m_handle;
		m_handle.idx = UINT16_MAX;
		return ret;
	}
};

class GraphicsDevice;
using GraphicsDeviceRef = Effekseer::RefPtr<GraphicsDevice>;

class GraphicsDevice : public Effekseer::Backend::GraphicsDevice {
private:
	RendererImplemented * m_render;
public:
	GraphicsDevice(RendererImplemented *render) : m_render(render) {};
	~GraphicsDevice() override = default;

	// For Renderer::Impl::CreateProxyTextures
	Effekseer::Backend::TextureRef CreateTexture(const Effekseer::Backend::TextureParameter& param, const Effekseer::CustomVector<uint8_t>& initialData) override;
	Effekseer::Backend::VertexLayoutRef CreateVertexLayout(const Effekseer::Backend::VertexLayoutElement* elements, int32_t elementCount) override {
		return Effekseer::MakeRefPtr<VertexLayout>(elements, elementCount);
	}
	std::string GetDeviceName() const override {
		return "BGFX";
	}
};

// Shader
class Shader : public EffekseerRenderer::ShaderBase {
	friend class RendererImplemented;
private:
	static const int maxUniform = 64;
	static const int maxTexture = 8;
	int m_vcbSize = 0;
	int m_pcbSize = 0;
	int m_vsSize = 0;
	int m_fsSize = 0;
	uint8_t * m_vcbBuffer = nullptr;
	uint8_t * m_pcbBuffer = nullptr;
	struct {
		bgfx_uniform_handle_t handle;
		int count;
		void * ptr;
	} m_uniform[maxUniform];
	bgfx_uniform_handle_t m_texture[maxTexture];
	bgfx_program_handle_t m_program;
	class RendererImplemented *m_render;
public:
	enum UniformType {
		Vertex,
		Pixel,
		Texture,
	};
	Shader(class RendererImplemented * render) : m_render(render) {};
	~Shader() override {
		delete[] m_vcbBuffer;
		delete[] m_pcbBuffer;
	}
	virtual void SetVertexConstantBufferSize(int32_t size) override {
		if (size > 0) {
			assert(m_vcbSize == 0);
			m_vcbSize = size;
			m_vcbBuffer = new uint8_t[size];
		}
	}
	virtual void SetPixelConstantBufferSize(int32_t size) override {
		if (size > 0) {
			assert(m_pcbSize == 0);
			m_pcbSize = size;
			m_pcbBuffer = new uint8_t[size];
		}
	}
	virtual void* GetVertexConstantBuffer() override {
		return m_vcbBuffer;
	}
	virtual void* GetPixelConstantBuffer() override {
		return m_pcbBuffer;
	}
	virtual void SetConstantBuffer() override;
	bool isValid() const {
		return m_render != nullptr;
	}
};

struct ShaderLoader {
	void *ud;
	bgfx_shader_handle_t (*loader)(const char *mat, const char *name, const char *type, void *ud);

	bgfx_shader_handle_t Load(const char *mat, const char *name, const char *type) const {
		return loader(mat, name, type, ud);
	}
};

class RenderState : public EffekseerRenderer::RenderStateBase {
private:
	RendererImplemented* m_renderer;
public:
	RenderState(RendererImplemented* renderer) : m_renderer(renderer) {}
	virtual ~RenderState() override = default;
	void Update(bool forced);
};

class TextureLoader : public Effekseer::TextureLoader {
private:
	const RendererImplemented *m_render;
	void *m_ud;
	bgfx_texture_handle_t (*m_loader)(const char *name, int srgb, void *ud);
	void (*m_unloader)(bgfx_texture_handle_t handle, void *ud);
public:
	TextureLoader(const RendererImplemented *render, InitArgs *init) : m_render(render) {
		m_ud = init->ud;
		m_loader = init->texture_load;
		m_unloader = init->texture_unload;
	}
	virtual ~TextureLoader() = default;
	Effekseer::TextureRef Load(const char16_t* path, Effekseer::TextureType textureType) override {
		char buffer[MAX_PATH];
		Effekseer::ConvertUtf16ToUtf8(buffer, MAX_PATH, path);
		int srgb = textureType == Effekseer::TextureType::Color;
		bgfx_texture_handle_t handle = m_loader(buffer, srgb, m_ud);

		auto texture = Effekseer::MakeRefPtr<Effekseer::Texture>();
		texture->SetBackend(Effekseer::MakeRefPtr<Texture>(m_render, handle));
		return texture;
	}
	void Unload(Effekseer::TextureRef texture) override {
		bgfx_texture_handle_t handle = texture->GetBackend().DownCast<Texture>()->RemoveInterface();
		m_unloader(handle, m_ud);
	}
};

class RendererImplemented : public Renderer, public Effekseer::ReferenceObject {
private:
	class StaticIndexBuffer : public Effekseer::Backend::IndexBuffer {
	private:
		const RendererImplemented * m_render;
		bgfx_index_buffer_handle_t m_buffer;
	public:
		StaticIndexBuffer(const RendererImplemented *render, bgfx_index_buffer_handle_t buffer)
			: m_render(render)
			, m_buffer(buffer) {}
		virtual ~StaticIndexBuffer() override {
			m_render->ReleaseIndexBuffer(this);
		}
		void UpdateData(const void* src, int32_t size, int32_t offset) override { assert(false); }	// Can't Update
		bgfx_index_buffer_handle_t GetInterface() const { return m_buffer; }
	};

	GraphicsDeviceRef m_device = nullptr;
	bgfx_interface_vtbl_t * m_bgfx = nullptr;
	EffekseerRenderer::RenderStateBase* m_renderState = nullptr;
	bool m_restorationOfStates = true;
	EffekseerRenderer::StandardRenderer<RendererImplemented, Shader>* m_standardRenderer = nullptr;
	EffekseerRenderer::DistortingCallback* m_distortingCallback = nullptr;
	StaticIndexBuffer* m_indexBuffer = nullptr;
	StaticIndexBuffer* m_indexBufferForWireframe = nullptr;
	TransientVertexBuffer* m_vertexBuffer = nullptr;
	Shader* m_currentShader = nullptr;
	int32_t m_squareMaxCount = 0;
	int32_t m_indexBufferStride = 2;
	bgfx_view_id_t m_viewid = 0;
	bgfx_vertex_layout_handle_t m_layout[SHADERCOUNT];
	bgfx_vertex_layout_t m_maxlayout;
	Shader * m_shaders[SHADERCOUNT];
	ShaderLoader m_shaderLoader;
	InitArgs m_initArgs;

	//! because gleDrawElements has only index offset
	int32_t GetIndexSpriteCount() const {
		int vsSize = EffekseerRenderer::GetMaximumVertexSizeInAllTypes() * m_squareMaxCount * 4;

		size_t size = sizeof(EffekseerRenderer::SimpleVertex);
		size = (std::min)(size, sizeof(EffekseerRenderer::DynamicVertex));
		size = (std::min)(size, sizeof(EffekseerRenderer::LightingVertex));

		return (int32_t)(vsSize / size / 4 + 1);
	}
	StaticIndexBuffer * CreateIndexBuffer(const bgfx_memory_t *mem, int stride) {
		bgfx_index_buffer_handle_t handle = BGFX(create_index_buffer)(mem, stride == 4 ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE);
		return new StaticIndexBuffer(this, handle);
	}
	void InitIndexBuffer() {
		int n = GetIndexSpriteCount();
		int i,j;
		const bgfx_memory_t *mem = BGFX(alloc)(n * 6 * m_indexBufferStride);
		uint8_t * ptr = mem->data;
		for (i=0;i<n;i++) {
			int buf[6] = {
				3 + 4 * i,
				1 + 4 * i,
				0 + 4 * i,
				3 + 4 * i,
				0 + 4 * i,
				2 + 4 * i,
			};
			if (m_indexBufferStride == 2) {
				uint16_t * dst = (uint16_t *)ptr;
				for (j=0;j<6;j++)
					dst[j] = (uint16_t)buf[j];
			} else {
				memcpy(ptr, buf, sizeof(buf));
			}
			ptr += 6 * m_indexBufferStride;
		}
		if (m_indexBuffer)
			delete m_indexBuffer;

		m_indexBuffer = CreateIndexBuffer(mem, m_indexBufferStride);

		mem = BGFX(alloc)(n * 8 * m_indexBufferStride);
		ptr = mem->data;

		for (i=0;i<n;i++) {
			int buf[8] = {
				0 + 4 * i,
				1 + 4 * i,
				2 + 4 * i,
				3 + 4 * i,
				0 + 4 * i,
				2 + 4 * i,
				1 + 4 * i,
				3 + 4 * i,
			};
			if (m_indexBufferStride == 2) {
				uint16_t * dst = (uint16_t *)ptr;
				for (j=0;j<8;j++)
					dst[j] = (uint16_t)buf[j];
			} else {
				memcpy(ptr, buf, sizeof(buf));
			}
			ptr += 8 * m_indexBufferStride;
		}
		if (m_indexBufferForWireframe)
			delete m_indexBufferForWireframe;
		m_indexBufferForWireframe = CreateIndexBuffer(mem, m_indexBufferStride);
	}
	void GenVertexLayout(bgfx_vertex_layout_t *layout, VertexLayoutRef v) const {
		const auto &elements = v->GetElements();
		BGFX(vertex_layout_begin)(layout, BGFX_RENDERER_TYPE_NOOP);
		for (int i = 0; i < elements.size(); i++) {
			const auto &e = elements[i];
			bgfx_attrib_t attrib = BGFX_ATTRIB_POSITION;
			uint8_t num = 0;
			bgfx_attrib_type_t type = BGFX_ATTRIB_TYPE_FLOAT;
			bool normalized = false;
			bool asInt = false;
			switch (e.Format) {
			case Effekseer::Backend::VertexLayoutFormat::R32_FLOAT :
				num = 1;
				type = BGFX_ATTRIB_TYPE_FLOAT;
				break;
			case Effekseer::Backend::VertexLayoutFormat::R32G32_FLOAT :
				num = 2;
				type = BGFX_ATTRIB_TYPE_FLOAT;
				break;
			case Effekseer::Backend::VertexLayoutFormat::R32G32B32_FLOAT :
				num = 3;
				type = BGFX_ATTRIB_TYPE_FLOAT;
				break;
			case Effekseer::Backend::VertexLayoutFormat::R32G32B32A32_FLOAT :
				num = 4;
				type = BGFX_ATTRIB_TYPE_FLOAT;
				break;
			case Effekseer::Backend::VertexLayoutFormat::R8G8B8A8_UNORM :
				num = 4;
				type = BGFX_ATTRIB_TYPE_UINT8;
				normalized = true;
				break;
			case Effekseer::Backend::VertexLayoutFormat::R8G8B8A8_UINT :
				num = 4;
				type = BGFX_ATTRIB_TYPE_UINT8;
				break;
			}
			if (e.SemanticName == "POSITION") {
				attrib = BGFX_ATTRIB_POSITION;
			} else if (e.SemanticName == "NORMAL") {
				switch (e.SemanticIndex) {
				case 0:
					attrib = BGFX_ATTRIB_COLOR0;
					break;
				case 1:
					attrib = BGFX_ATTRIB_NORMAL;
					break;
				case 2:
					attrib = BGFX_ATTRIB_TANGENT;
					break;
				case 3:
					attrib = BGFX_ATTRIB_BITANGENT;
					break;
				case 4:
					attrib = BGFX_ATTRIB_COLOR1;
					break;
				case 5:
					attrib = BGFX_ATTRIB_COLOR2;
					break;
				default:
					attrib = BGFX_ATTRIB_COLOR3;
					break;
				}
			} else if (e.SemanticName == "TEXCOORD") {
				attrib = (bgfx_attrib_t)((int)BGFX_ATTRIB_TEXCOORD0 + e.SemanticIndex);
			}
			BGFX(vertex_layout_add)(layout, attrib, num, type, normalized, asInt);
		}
		BGFX(vertex_layout_end)(layout);
	}
	void InitVertexBuffer() {
		bgfx_vertex_layout_t layout[SHADERCOUNT];
		for (auto t : {
			EffekseerRenderer::RendererShaderType::Unlit,
			EffekseerRenderer::RendererShaderType::Lit,
			EffekseerRenderer::RendererShaderType::BackDistortion,
			EffekseerRenderer::RendererShaderType::AdvancedUnlit,
			EffekseerRenderer::RendererShaderType::AdvancedLit,
			EffekseerRenderer::RendererShaderType::AdvancedBackDistortion,
		}) {
			int id = (int)t;
			GenVertexLayout(&layout[id], EffekseerRenderer::GetVertexLayout(m_device, t).DownCast<VertexLayout>());
		}
		int maxstride = 0;
		int maxstride_id = 0;
		for (int i=0;i<SHADERCOUNT;i++) {
			m_layout[i] = BGFX(create_vertex_layout)(&layout[i]);
			if (layout[i].stride > maxstride) {
				maxstride = layout[i].stride;
				maxstride_id = i;
			}
		}
		m_maxlayout = layout[maxstride_id];
		m_vertexBuffer = new TransientVertexBuffer(m_squareMaxCount * maxstride);
	}
	bool InitShaders(struct InitArgs *init) {
		m_initArgs = *init;
		m_shaderLoader.ud = init->ud;
		m_shaderLoader.loader = init->shader_load;
		static const char *shadername[SHADERCOUNT] = {
			"sprite_unlit",
			"sprite_lit",
			"sprite_distortion",
			"sprite_adv_unlit",
			"sprite_adv_lit",
			"sprite_adv_distortion",
		};
		int i;
		for (i=0;i<SHADERCOUNT;i++) {
			Shader * s = new Shader(this);
			m_shaders[i] = s;
			InitShader(s,
				m_shaderLoader.Load(NULL, shadername[i], "vs"),
				m_shaderLoader.Load(NULL, shadername[i], "fs"));
			s->SetVertexConstantBufferSize(sizeof(EffekseerRenderer::StandardRendererVertexBuffer));
			AddUniform(s, "u_UVInversed", Shader::UniformType::Vertex,
				offsetof(EffekseerRenderer::StandardRendererVertexBuffer, uvInversed));
			AddUniform(s, "u_vsFlipbookParameter", Shader::UniformType::Vertex,
				offsetof(EffekseerRenderer::StandardRendererVertexBuffer, flipbookParameter));
			AddUniform(s, "s_sampler_colorTex", Shader::UniformType::Texture, 0);
		}
		for (i=0;i<SHADERCOUNT;i++) {
			Shader * s = m_shaders[i];
			if (!s->isValid())
				return false;
		}
		for (auto t: {
			EffekseerRenderer::RendererShaderType::Unlit,
			EffekseerRenderer::RendererShaderType::Lit,
			EffekseerRenderer::RendererShaderType::AdvancedUnlit,
			EffekseerRenderer::RendererShaderType::AdvancedLit,
		}) {
			int id = (int)t;
			Shader * s = m_shaders[id];
			s->SetPixelConstantBufferSize(sizeof(EffekseerRenderer::PixelConstantBuffer));
#define PIXELUNIFORM(name) AddUniform(s, "f" #name, Shader::UniformType::Pixel, offsetof(EffekseerRenderer::PixelConstantBuffer, name));
			PIXELUNIFORM(LightDirection)
			PIXELUNIFORM(LightColor)
			PIXELUNIFORM(LightAmbientColor)
			PIXELUNIFORM(FlipbookParam)
			PIXELUNIFORM(UVDistortionParam)
			PIXELUNIFORM(BlendTextureParam)
			PIXELUNIFORM(CameraFrontDirection)
			PIXELUNIFORM(FalloffParam)
			PIXELUNIFORM(EmmisiveParam)
			PIXELUNIFORM(EdgeParam)
			PIXELUNIFORM(SoftParticleParam)
			PIXELUNIFORM(UVInversedBack)
			PIXELUNIFORM(MiscFlags)
#undef PIXELUNIFORM
		}
		for (auto t: {
			EffekseerRenderer::RendererShaderType::BackDistortion,
			EffekseerRenderer::RendererShaderType::AdvancedBackDistortion,
		}) {
			int id = (int)t;
			Shader * s = m_shaders[id];
			s->SetPixelConstantBufferSize(sizeof(EffekseerRenderer::PixelConstantBufferDistortion));
#define PIXELDUNIFORM(name) AddUniform(s, "f" #name, Shader::UniformType::Pixel, offsetof(EffekseerRenderer::PixelConstantBufferDistortion, name));
			PIXELDUNIFORM(DistortionIntencity)
			PIXELDUNIFORM(UVInversedBack)
			PIXELDUNIFORM(FlipbookParam)
			PIXELDUNIFORM(BlendTextureParam)
			PIXELDUNIFORM(SoftParticleParam)
#undef PIXELDUNIFORM
		}
		return true;
	}
public:
	RendererImplemented() {
		m_device = Effekseer::MakeRefPtr<GraphicsDevice>(this);
		int i;
		for (i=0;i<SHADERCOUNT;i++) {
			m_layout[i].idx = UINT16_MAX;
		}
		for (i=0;i<SHADERCOUNT;i++) {
			m_shaders[i] = nullptr;
		}
	}
	~RendererImplemented() {
		GetImpl()->DeleteProxyTextures(this);

		ES_SAFE_DELETE(m_distortingCallback);
		ES_SAFE_DELETE(m_standardRenderer);
		ES_SAFE_DELETE(m_renderState);
		for (auto shader : m_shaders) {
			ReleaseShader(shader);
			ES_SAFE_DELETE(shader);
		}
		int i;
		for (i=0;i<SHADERCOUNT;i++) {
			BGFX(destroy_vertex_layout)(m_layout[i]);
		}
		ES_SAFE_DELETE(m_indexBuffer);
		ES_SAFE_DELETE(m_indexBufferForWireframe);
		ES_SAFE_DELETE(m_vertexBuffer);
	}

	void OnLostDevice() override {}
	void OnResetDevice() override {}

	bool Initialize(struct InitArgs *init) {
		m_bgfx = init->bgfx;
		if (!InitShaders(init)) {
			return false;
		}
		m_viewid = init->viewid;
		m_squareMaxCount = init->squareMaxCount;
		if (GetIndexSpriteCount() * 4 > 65536) {
			m_indexBufferStride = 4;
		}
		InitIndexBuffer();
		InitVertexBuffer();
		m_renderState = new RenderState(this);
		
		m_standardRenderer = new EffekseerRenderer::StandardRenderer<RendererImplemented, Shader>(this);

		GetImpl()->isSoftParticleEnabled = true;
		GetImpl()->CreateProxyTextures(this);
		return true;
	}
	void SetRestorationOfStatesFlag(bool flag) override {
		m_restorationOfStates = flag;
	}
	bool BeginRendering() override {
		// Alloc TransientVertexBuffer
		bgfx_transient_vertex_buffer_t * tvb = m_vertexBuffer->GetInterface();
		BGFX(alloc_transient_vertex_buffer)(tvb, m_squareMaxCount, &m_maxlayout);

		GetImpl()->CalculateCameraProjectionMatrix();

		// todo:	currentTextures_.clear();
		m_renderState->GetActiveState().Reset();
		// todo:	m_renderState->Update(true);
		m_renderState->GetActiveState().TextureIDs.fill(0);

		m_standardRenderer->ResetAndRenderingIfRequired();
		return true;
	}
	bool EndRendering() override {
		m_standardRenderer->ResetAndRenderingIfRequired();
		return true;
	}
	TransientVertexBuffer* GetVertexBuffer() {
		return m_vertexBuffer;
	}
	StaticIndexBuffer* GetIndexBuffer() {
		return m_indexBuffer;
	}
	int32_t GetSquareMaxCount() const override {
		return m_squareMaxCount;
	}
	void SetSquareMaxCount(int32_t count) override {
		m_squareMaxCount = count;
		InitIndexBuffer();
	}
	EffekseerRenderer::RenderStateBase* GetRenderState() {
		return m_renderState;
	}

	Effekseer::SpriteRendererRef CreateSpriteRenderer() override {
		return Effekseer::SpriteRendererRef(new EffekseerRenderer::SpriteRendererBase<RendererImplemented, false>(this));
	}
	Effekseer::RibbonRendererRef CreateRibbonRenderer() override {
		return Effekseer::RibbonRendererRef(new EffekseerRenderer::RibbonRendererBase<RendererImplemented, false>(this));
	}
	Effekseer::RingRendererRef CreateRingRenderer() override {
		return Effekseer::RingRendererRef(new EffekseerRenderer::RingRendererBase<RendererImplemented, false>(this));
	}
	Effekseer::ModelRendererRef CreateModelRenderer() override {
		// todo: return ModelRenderer::Create(this);
		return nullptr;
	}
	Effekseer::TrackRendererRef CreateTrackRenderer() override {
		return Effekseer::TrackRendererRef(new EffekseerRenderer::TrackRendererBase<RendererImplemented, false>(this));
	}
	Effekseer::TextureLoaderRef CreateTextureLoader(Effekseer::FileInterfaceRef fileInterface = nullptr) {
		return Effekseer::MakeRefPtr<TextureLoader>(this, &m_initArgs);
	}
	Effekseer::ModelLoaderRef CreateModelLoader(::Effekseer::FileInterfaceRef fileInterface = nullptr) {
		// todo: return Effekseer::MakeRefPtr<ModelLoader>(m_device, fileInterface);
		return nullptr;
	}
	Effekseer::MaterialLoaderRef CreateMaterialLoader(::Effekseer::FileInterfaceRef fileInterface = nullptr) {
		// todo: return Effekseer::MakeRefPtr<MaterialLoader>(m_device, fileInterface);
		return nullptr;
	}
	EffekseerRenderer::DistortingCallback* GetDistortingCallback() override {
		return m_distortingCallback;
	}
	void SetDistortingCallback(EffekseerRenderer::DistortingCallback* callback) override {
		ES_SAFE_DELETE(m_distortingCallback);
		m_distortingCallback = callback;
	}
	EffekseerRenderer::StandardRenderer<RendererImplemented, Shader>* GetStandardRenderer() {
		return m_standardRenderer;
	}
	void SetVertexBuffer(TransientVertexBuffer* vertexBuffer, int32_t stride) {
		auto shaderType = m_standardRenderer->GetState().Collector.ShaderType;
		int id = (int)shaderType;
		if (shaderType == EffekseerRenderer::RendererShaderType::Material) {
			// todo : materaial
			return;
		}
		BGFX(set_transient_vertex_buffer_with_layout)(0, vertexBuffer->GetInterface(), 0, vertexBuffer->GetSize()/stride, m_layout[id]);
	}
	void SetIndexBuffer(StaticIndexBuffer* indexBuffer) {
		BGFX(set_index_buffer)(indexBuffer->GetInterface(), 0, UINT32_MAX);
	}
	void SetLayout(Shader* shader) {
		// todo:
	}
	void DrawSprites(int32_t spriteCount, int32_t vertexOffset) {
		// todo:
	}
	void DrawPolygon(int32_t vertexCount, int32_t indexCount) {
		// todo:
	}
	void DrawPolygonInstanced(int32_t vertexCount, int32_t indexCount, int32_t instanceCount) {
		// todo:
	}
	Shader* GetShader(EffekseerRenderer::RendererShaderType type) const {
		int n = (int)type;
		if (n<0 || n>= SHADERCOUNT)
			return nullptr;
		return m_shaders[n];
	}
	void BeginShader(Shader* shader) {
		assert(m_currentShader == nullptr);
		m_currentShader = shader;
	}
	void EndShader(Shader* shader) {
		assert(m_currentShader == shader);
		m_currentShader = nullptr;
	}
	void SetVertexBufferToShader(const void* data, int32_t size, int32_t dstOffset) {
		assert(m_currentShader != nullptr);
		auto p = static_cast<uint8_t*>(m_currentShader->GetVertexConstantBuffer()) + dstOffset;
		memcpy(p, data, size);
	}
	void SetPixelBufferToShader(const void* data, int32_t size, int32_t dstOffset) {
		assert(m_currentShader != nullptr);
		auto p = static_cast<uint8_t*>(m_currentShader->GetPixelConstantBuffer()) + dstOffset;
		memcpy(p, data, size);
	}
	void SetTextures(Shader* shader, Effekseer::Backend::TextureRef* textures, int32_t count) {
		// todo:
	}
	void ResetRenderState() override {
		m_renderState->GetActiveState().Reset();
		m_renderState->Update(true);
	}
	void SetCurrentState(uint64_t state) {
		BGFX(set_state)(state, 0);
	}
	Effekseer::Backend::GraphicsDeviceRef GetGraphicsDevice() const override {
		return m_device;
	}
	virtual int GetRef() override { return Effekseer::ReferenceObject::GetRef(); }
	virtual int AddRef() override { return Effekseer::ReferenceObject::AddRef(); }
	virtual int Release() override { return Effekseer::ReferenceObject::Release(); }

	// Shader API
	void InitShader(Shader *s, bgfx_shader_handle_t vs, bgfx_shader_handle_t fs) {
		s->m_program = BGFX(create_program)(vs, fs, false);
		if (s->m_program.idx == UINT16_MAX) {
			s->m_render = nullptr;
			return;
		}
		bgfx_uniform_handle_t u[Shader::maxUniform];
		s->m_vsSize = BGFX(get_shader_uniforms)(vs, u, Shader::maxUniform);
		int i;
		for (i=0;i<s->m_vsSize;i++) {
			s->m_uniform[i].handle = u[i];
			s->m_uniform[i].count = 0;
			s->m_uniform[i].ptr = nullptr;
		}
		s->m_fsSize = BGFX(get_shader_uniforms)(fs, u, Shader::maxUniform - s->m_vsSize);
		for (i=0;i<s->m_fsSize;i++) {
			s->m_uniform[i+s->m_vsSize].handle = u[i];
			s->m_uniform[i+s->m_vsSize].count = 0;
			s->m_uniform[i+s->m_vsSize].ptr = nullptr;
		}
		for (i=0;i<Shader::maxTexture;i++) {
			s->m_texture[i].idx = UINT16_MAX;
		}
	}
	void ReleaseShader(Shader *s) {
		if (s->isValid()) {
			BGFX(destroy_program)(s->m_program);
			s->m_render = nullptr;
		}
	}
	void SumbitUniforms(Shader *s) {
		if (!s->isValid())
			return;
		BGFX(set_view_transform)(m_viewid, s->m_vcbBuffer, s->m_vcbBuffer + sizeof(Effekseer::Matrix44));
		int i;
		for (i=0;i<s->m_vsSize + s->m_fsSize;i++) {
			if (s->m_uniform[i].ptr != nullptr) {
				BGFX(set_uniform)(s->m_uniform[i].handle, s->m_uniform[i].ptr, s->m_uniform[i].count);
			}
		}
	}
	void AddUniform(Shader *s, const char *name, Shader::UniformType type, int offset) {
		if (!s->isValid())
			return;
		int i;
		int from = 0;
		int	to = s->m_vsSize + s->m_fsSize;
		switch(type) {
		case Shader::UniformType::Vertex:
			to = s->m_vsSize;
			break;
		case Shader::UniformType::Pixel:
			from = s->m_vsSize;
			to = s->m_vsSize + s->m_fsSize;
			break;
		default:
			break;
		}
		bgfx_uniform_info_t info;
		for (i=from;i<to;i++) {
			if (s->m_uniform[i].count == 0) {
				info.name[0] = 0;
				BGFX(get_uniform_info)(s->m_uniform[i].handle, &info);
				if (strcmp(info.name, name) == 0) {
					break;
				}
			}
		}

		if (i >= to) {
			ReleaseShader(s);
			return;
		}

		switch(type) {
		case Shader::UniformType::Vertex:
			s->m_uniform[i].ptr = s->m_vcbBuffer + offset;
			s->m_uniform[i].count = info.num;
			break;
		case Shader::UniformType::Pixel:
			s->m_uniform[i].ptr = s->m_pcbBuffer + offset;
			s->m_uniform[i].count = info.num;
			break;
		case Shader::UniformType::Texture:
			assert(info.type == BGFX_UNIFORM_TYPE_SAMPLER);
			assert(offset >= 0 && offset < Shader::maxTexture);
			s->m_uniform[i].count = offset + 1;
			assert(s->m_texture[offset].idx == UINT16_MAX);
			s->m_texture[offset] = s->m_uniform[i].handle;
			break;
		}
	}

	Effekseer::Backend::TextureRef CreateTexture(const Effekseer::Backend::TextureParameter& param, const Effekseer::CustomVector<uint8_t>& initialData) {
		// Only for CreateProxyTexture, See EffekseerRendererCommon/EffekseerRenderer.Renderer.cpp
		assert(param.Format == Effekseer::Backend::TextureFormatType::R8G8B8A8_UNORM);
		assert(param.Dimension == 2);

		const bgfx_memory_t *mem = BGFX(copy)(initialData.data(), initialData.size());
		bgfx_texture_handle_t handle = BGFX(create_texture_2d)(param.Size[0], param.Size[1], false, 1, BGFX_TEXTURE_FORMAT_RGBA8,
			BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, mem);

		return Effekseer::MakeRefPtr<Texture>(this, handle);
	}
	void ReleaseTexture(Texture *t) const {
		BGFX(destroy_texture)(t->RemoveInterface());
	}
	void ReleaseIndexBuffer(StaticIndexBuffer *ib) const {
		BGFX(destroy_index_buffer)(ib->GetInterface());
	}
private:
//	void DoDraw();
};

void Shader::SetConstantBuffer() {
	m_render->SumbitUniforms(this);
}

void RenderState::Update(bool forced) {
	(void)forced;	// ignore forced
	uint64_t state = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_FRONT_CCW
		| BGFX_STATE_MSAA;

	if (m_next.DepthTest) {
		state |= BGFX_STATE_DEPTH_TEST_LEQUAL;
	} else {
		state |= BGFX_STATE_DEPTH_TEST_ALWAYS;
	}

	if (m_next.DepthWrite) {
		state |= BGFX_STATE_WRITE_Z;
	}

	// isCCW
	if (m_next.CullingType == Effekseer::CullingType::Front) {
		state |= BGFX_STATE_CULL_CW;
	}
	else if (m_next.CullingType == Effekseer::CullingType::Back) {
		state |= BGFX_STATE_CULL_CCW;
	}
	if (m_next.AlphaBlend == ::Effekseer::AlphaBlendType::Opacity ||
		m_renderer->GetRenderMode() == ::Effekseer::RenderMode::Wireframe) {
			state |= BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_ADD, BGFX_STATE_BLEND_EQUATION_MAX);
			state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
		}
	else if (m_next.AlphaBlend == ::Effekseer::AlphaBlendType::Sub)	{
		state |= BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_REVSUB, BGFX_STATE_BLEND_EQUATION_ADD);
		state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE);
	} else {
		if (m_next.AlphaBlend == ::Effekseer::AlphaBlendType::Blend) {
			state |= BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_ADD, BGFX_STATE_BLEND_EQUATION_MAX);
			state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
		} else if (m_next.AlphaBlend == ::Effekseer::AlphaBlendType::Add) {
			state |= BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_ADD, BGFX_STATE_BLEND_EQUATION_MAX);
			state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
		} else if (m_next.AlphaBlend == ::Effekseer::AlphaBlendType::Mul) {
			state |= BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_ADD, BGFX_STATE_BLEND_EQUATION_ADD);
			state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_SRC_COLOR, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE);
		}
	}
	m_renderer->SetCurrentState(state);
	m_active = m_next;
}

Effekseer::Backend::TextureRef GraphicsDevice::CreateTexture(const Effekseer::Backend::TextureParameter& param, const Effekseer::CustomVector<uint8_t>& initialData) {
	return m_render->CreateTexture(param, initialData);
}

Texture::~Texture() {
	m_render->ReleaseTexture(this);
}

// Create Renderer
RendererRef Renderer::Create(struct InitArgs *init) {
	auto renderer = Effekseer::MakeRefPtr<RendererImplemented>();
	if (renderer->Initialize(init))	{
		return renderer;
	}
	return nullptr;
}

}
