/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bx/uint32_t.h>
#include <bx/math.h>
#include <bx/readerwriter.h>

#include <common.h>
#include <bgfx_utils.h>
#include <bgfx/c99/bgfx.h>

#include "renderer/bgfxrenderer.h"
#define DEF_VIEWID	0

#include <string>
namespace
{

class EffekseerBgfxTest : public entry::AppI
{
public:
	EffekseerBgfxTest(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_TEXT;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type     = args.m_type;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		auto inter = bgfx_get_interface(BGFX_API_VERSION);

		EffekseerRendererBGFX::InitArgs efkArgs {
			2048, DEF_VIEWID, inter,
			EffekseerBgfxTest::ShaderLoad,
			EffekseerBgfxTest::TextureLoad,
			EffekseerBgfxTest::TextureGet,
			EffekseerBgfxTest::TextureUnload,
			nullptr
		};

		m_efkRenderer = EffekseerRendererBGFX::CreateRenderer(&efkArgs);
		m_efkManager = Effekseer::Manager::Create(8000);
		m_efkManager->GetSetting()->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

		m_efkManager->SetModelRenderer(CreateModelRenderer(m_efkRenderer, &efkArgs));
		m_efkManager->SetSpriteRenderer(m_efkRenderer->CreateSpriteRenderer());
		m_efkManager->SetRibbonRenderer(m_efkRenderer->CreateRibbonRenderer());
		m_efkManager->SetRingRenderer(m_efkRenderer->CreateRingRenderer());
		m_efkManager->SetTrackRenderer(m_efkRenderer->CreateTrackRenderer());
		m_efkManager->SetTextureLoader(m_efkRenderer->CreateTextureLoader());
		m_efkManager->SetModelLoader(m_efkRenderer->CreateModelLoader());
		m_efkManager->SetMaterialLoader(m_efkRenderer->CreateMaterialLoader());
		m_efkManager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

		m_efkRenderer->SetProjectionMatrix(Effekseer::Matrix44().PerspectiveFovLH(
			bx::toRad(90.0f), m_width/float(m_height), 1.0f, 500.0f));
		m_efkRenderer->SetCameraMatrix(
		Effekseer::Matrix44().LookAtLH(Effekseer::Vector3D(10.0f, 5.0f, -20.0f), Effekseer::Vector3D(0.0f, 0.0f, 0.0f), Effekseer::Vector3D(0.0f, 1.0f, 0.0f)));

		//m_efkEffect = Effekseer::Effect::Create(m_efkManager, u"./resources/Simple_Model_UV.efkefc");
		m_efkEffect = Effekseer::Effect::Create(m_efkManager, u"./resources/Laser01.efk");

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
			);
	}

	virtual int shutdown() override
	{
		m_efkEffect = nullptr;
		m_efkManager = nullptr;
		m_efkRenderer = nullptr;
		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			// Set view 0 default viewport.
			bgfx::setViewRect(DEF_VIEWID, 0, 0, uint16_t(m_width), uint16_t(m_height) );

			static int s_time = 0;
			static Effekseer::Handle s_handle = 0;

			if (s_time % 120 == 0){
				s_handle = m_efkManager->Play(m_efkEffect, 0, 0, 0);
			} else if (s_time % 120 == 119){
				m_efkManager->StopEffect(s_handle);
			}

			m_efkManager->AddLocation(s_handle, Effekseer::Vector3D(0.2f, 0.0f, 0.0f));
			m_efkManager->Update();

			m_efkRenderer->SetTime(s_time / 60.0f);
			m_efkRenderer->BeginRendering();

			Effekseer::Manager::DrawParameter drawParameter;
			drawParameter.ZNear = 0.0f;
			drawParameter.ZFar = 1.0f;
			drawParameter.ViewProjectionMatrix = m_efkRenderer->GetCameraProjectionMatrix();
			m_efkManager->Draw(drawParameter);

			m_efkRenderer->EndRendering();
			bgfx::frame();

			++s_time;
			return true;
		}

		return false;
	}

	entry::MouseState m_mouseState;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

private:
	static const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const char* _filePath)
	{
		if (bx::open(_reader, _filePath) )
		{
			uint32_t size = (uint32_t)bx::getSize(_reader);
			const bgfx::Memory* mem = bgfx::alloc(size+1);
			bx::read(_reader, mem->data, size, bx::ErrorAssert{});
			bx::close(_reader);
			mem->data[mem->size-1] = '\0';
			return mem;
		}

		DBG("Failed to load %s.", _filePath);
		return NULL;
	}

	static const char*
	findShaderFile(const char* name, const char* type){
#define CHECK_SHADER(_SHADERNAME, _VS, _FS)	if (strcmp(name, _SHADERNAME) == 0){	\
			if (strcmp(type, "vs") == 0){\
				return _VS;\
			}\
			assert(strcmp(type, "fs") == 0);\
			return _FS;\
		}

		CHECK_SHADER("sprite_unlit", 			"shaders/sprite_unlit_vs.fx.bin", 		"shaders/model_unlit_ps.fx.bin");
		CHECK_SHADER("sprite_lit", 				"shaders/sprite_lit_vs.fx.bin", 		"shaders/model_lit_ps.fx.bin");
		CHECK_SHADER("sprite_distortion", 		"shaders/sprite_distortion_vs.fx.bin", 	"shaders/model_distortion_ps.fx.bin");
		CHECK_SHADER("sprite_adv_unlit", 		"shaders/ad_sprite_unlit_vs.fx.bin", 	"shaders/ad_model_unlit_ps.fx.bin");
		CHECK_SHADER("sprite_adv_lit", 			"shaders/ad_sprite_lit_vs.fx.bin", 		"shaders/ad_model_lit_ps.fx.bin");
		CHECK_SHADER("sprite_adv_distortion", 	"shaders/ad_sprite_distortion_vs.fx.bin","shaders/ad_model_distortion_ps.fx.bin");

		CHECK_SHADER("model_unlit", 			"shaders/model_unlit_vs.fx.bin", 		"shaders/model_unlit_ps.fx.bin");
		CHECK_SHADER("model_lit", 				"shaders/model_lit_vs.fx.bin", 			"shaders/model_lit_ps.fx.bin");
		CHECK_SHADER("model_distortion", 		"shaders/model_distortion_vs.fx.bin", 	"shaders/model_distortion_ps.fx.bin");
		CHECK_SHADER("model_adv_unlit", 		"shaders/ad_model_unlit_vs.fx.bin", 	"shaders/ad_model_unlit_ps.fx.bin");
		CHECK_SHADER("model_adv_lit", 			"shaders/ad_model_lit_vs.fx.bin", 		"shaders/ad_model_lit_ps.fx.bin");
		CHECK_SHADER("model_adv_distortion", 	"shaders/ad_model_distortion_vs.fx.bin","shaders/ad_model_distortion_ps.fx.bin");

		assert(false && "invalid shader name and type name");
		return nullptr;
	}

	static bgfx_shader_handle_t ShaderLoad(const char *mat, const char *name, const char *type, void *ud){
		assert(mat == nullptr);
		const char* shaderfile = findShaderFile(name, type);
		bgfx::ShaderHandle handle = bgfx::createShader(loadMem(entry::getFileReader(), shaderfile) );
		bgfx::setName(handle, shaderfile);
		
		return bgfx_shader_handle_t{handle.idx};
	}
	static bool isPngFile(const char* filename){
		std::string s(filename);
		for (auto &c: s) c = std::tolower(c);
		return s.rfind(".png") != std::string::npos;
	}

	static bgfx::TextureHandle createTexture(const char* filename, uint64_t state){
		if (isPngFile(filename)){
			auto image = imageLoad(filename, bgfx::TextureFormat::BGRA8);
			assert(image && "invalid png file");
			auto h = bgfx::createTexture2D(
				  (uint16_t)image->m_width
				, (uint16_t)image->m_height
				, false
				, 1
				, bgfx::TextureFormat::BGRA8
				, state
				, bgfx::copy(image->m_data, image->m_size)
				);
			imageFree(image);
			return h;
		}

		return bgfx::createTexture(loadMem(entry::getFileReader(), filename), state);
	}

	static bgfx_texture_handle_t TextureGet(int texture_type, void *parm, void *ud){
		assert(false && "not implement");
		return {BGFX_INVALID_HANDLE};
	}

	static bgfx_texture_handle_t TextureLoad(const char *name, int srgb, void *ud){
		const uint64_t state = (srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE)|BGFX_SAMPLER_NONE;
		auto handle = createTexture(name, state);
		return {handle.idx};
	}

	static void TextureUnload(bgfx_texture_handle_t handle, void *ud){
		bgfx::TextureHandle h = {handle.idx};
		bgfx::destroy(h);
	}
private:
	EffekseerRenderer::RendererRef m_efkRenderer = nullptr;
	Effekseer::ManagerRef m_efkManager = nullptr;
	Effekseer::EffectRef m_efkEffect = nullptr;
};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	  EffekseerBgfxTest
	, "EffekseerBgfxTest"
	, "Effekseer bgfx renderer example."
	, "https://github.com/cloudwu/efkbgfx"
	);
