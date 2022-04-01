/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bx/uint32_t.h>
#include <bx/math.h>
#include <bx/readerwriter.h>

#include <bimg/decode.h>

#include <common.h>
#include <bgfx_utils.h>
#include <bgfx/c99/bgfx.h>

#include "renderer/bgfxrenderer.h"

static const bgfx::ViewId g_sceneViewId = 0;
static const bgfx::ViewId g_defaultViewId = 1;

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

		bgfx::setViewMode(g_sceneViewId, bgfx::ViewMode::Sequential);
		bgfx::setViewMode(g_defaultViewId, bgfx::ViewMode::Sequential);

		auto inter = bgfx_get_interface(BGFX_API_VERSION);

		EffekseerRendererBGFX::InitArgs efkArgs {
			2048, g_defaultViewId, inter,
			EffekseerBgfxTest::ShaderLoad,
			EffekseerBgfxTest::TextureLoad,
			EffekseerBgfxTest::TextureGet,
			EffekseerBgfxTest::TextureUnload,
			this,
		};

		initFullScreen();
		initCube();

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

		m_projMat.PerspectiveFovLH(
			bx::toRad(90.0f), m_width/float(m_height), 1.0f, 500.0f);
		m_efkRenderer->SetProjectionMatrix(m_projMat);
		m_viewMat.LookAtLH(Effekseer::Vector3D(0.0f, 0.0f, 40.0f), Effekseer::Vector3D(0.0f, 0.0f, 0.0f), Effekseer::Vector3D(0.0f, 1.0f, 0.0f));
		m_efkRenderer->SetCameraMatrix(m_viewMat);

		//m_efkEffect = Effekseer::Effect::Create(m_efkManager, u"./resources/Simple_Model_UV.efkefc");
		//m_efkEffect = Effekseer::Effect::Create(m_efkManager, u"./resources/Laser01.efk");
		m_efkEffect = Effekseer::Effect::Create(m_efkManager, u"./resources/sword_lightning.efkefc");
		m_efkEffectHandle = m_efkManager->Play(m_efkEffect, 0, 0, 0);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(g_defaultViewId
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
			bgfx::setViewRect(g_defaultViewId, 0, 0, uint16_t(m_width), uint16_t(m_height) );
			m_efkManager->Update();

			drawCube(g_sceneViewId);
			drawFullScreen(g_defaultViewId);

			bgfx::setViewTransform(g_defaultViewId, m_viewMat.Values, m_projMat.Values);
			//m_efkRenderer->SetTime(s_time / 60.0f);
			m_efkRenderer->BeginRendering();

			Effekseer::Manager::DrawParameter drawParameter;
			drawParameter.ZNear = 0.0f;
			drawParameter.ZFar = 1.0f;
			drawParameter.ViewProjectionMatrix = m_efkRenderer->GetCameraProjectionMatrix();
			m_efkManager->Draw(drawParameter);

			m_efkRenderer->EndRendering();
			bgfx::frame();
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
	void
	initCube(){
		struct PosColorVertex
		{
			float m_x;
			float m_y;
			float m_z;
			uint32_t m_abgr;
		};
		const float len = 5.f;
		static const PosColorVertex s_vertices[] =
		{
			{-len,  len,  len, 0xff000000 },
			{ len,  len,  len, 0xff0000ff },
			{-len, -len,  len, 0xff00ff00 },
			{ len, -len,  len, 0xff00ffff },
			{-len,  len, -len, 0xffff0000 },
			{ len,  len, -len, 0xffff00ff },
			{-len, -len, -len, 0xffffff00 },
			{ len, -len, -len, 0xffffffff },
		};

		static const uint16_t s_indices[] =
		{
			0, 1, 2, // 0
			1, 3, 2,
			4, 6, 5, // 2
			5, 6, 7,
			0, 2, 4, // 4
			4, 2, 6,
			1, 5, 3, // 6
			5, 7, 3,
			0, 4, 1, // 8
			4, 5, 1,
			2, 3, 6, // 10
			6, 3, 7,
		};

		m_cubeLayout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
		
		m_cubeVertexBuffer = bgfx::createVertexBuffer(
			bgfx::makeRef(s_vertices, sizeof(s_vertices))
			,m_cubeLayout);

		m_cubeIndexBuffer = bgfx::createIndexBuffer(
			bgfx::makeRef(s_indices, sizeof(s_indices) ));


		m_cubeProg = bgfx::createProgram(
			bgfx::createShader(loadMem(entry::getFileReader(), "cube/shaders/vs_cube.bin") ),
			bgfx::createShader(loadMem(entry::getFileReader(), "cube/shaders/fs_cube.bin") ),
			true);
	}

	void
	drawCube(bgfx::ViewId viewid){
		bgfx::setViewFrameBuffer(viewid, m_frameBuffer);
		const float h = 15.f;
		const float worldmat[16] = {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, h,   0.f, 1.f,
		};
		bgfx::setTransform(worldmat);
		bgfx::setViewTransform(viewid, m_viewMat.Values, m_projMat.Values);
		bgfx::setVertexBuffer(0, m_cubeVertexBuffer);
		bgfx::setIndexBuffer(m_cubeIndexBuffer);

		bgfx::setState(m_renderstate);
		bgfx::submit(viewid, m_cubeProg, 0, BGFX_DISCARD_ALL);
	}

	void
	initFullScreen(){
		m_sceneTex 	= bgfx::createTexture2D(m_width, m_height, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
		m_sceneDepth = bgfx::createTexture2D(m_width, m_height, false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT);
		const bgfx::TextureHandle handles[] = {m_sceneTex, m_sceneDepth};
		m_frameBuffer = bgfx::createFrameBuffer(2, handles, false);

		bgfx::setViewFrameBuffer(g_sceneViewId, m_frameBuffer);
		bgfx::setViewRect(g_sceneViewId, 0, 0, m_width, m_height);
		bgfx::setViewClear(g_sceneViewId
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x000000ff
			, 1.0f
			, 0
			);

		bgfx::ShaderHandle fs = bgfx::createShader(loadMem(entry::getFileReader(), "cube/shaders/fs_fullscreen.bin"));
		m_fullscreenProg = bgfx::createProgram(
			bgfx::createShader(loadMem(entry::getFileReader(), "cube/shaders/vs_fullscreen.bin")),
			fs,
			true);

		bgfx::UniformHandle uniforms[16];
		uint32_t numUniform = bgfx::getShaderUniforms(fs, uniforms, 16);
		for (uint32_t ii=0; ii<numUniform; ++ii){
			bgfx::UniformInfo info;
			const bgfx::UniformHandle h = uniforms[ii];
			bgfx::getUniformInfo(h, info);
			if (info.name == "s_scene"){
				m_fullscreenTextureUniformHandle = h;
				break;
			}
		}
	}

	void
	drawFullScreen(bgfx::ViewId viewid) {
		bgfx::setVertexBuffer(0, m_cubeVertexBuffer, 0, 3);
		bgfx::setState(m_fullscreenstate);
		bgfx::setTexture(0, m_fullscreenTextureUniformHandle, m_sceneTex, BGFX_SAMPLER_NONE);
		bgfx::submit(viewid, m_fullscreenProg, 0, BGFX_DISCARD_ALL);
	}

	static const bgfx::Memory*
	loadMem(bx::FileReaderI* _reader, const char* _filePath) {
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

		CHECK_SHADER("sprite_unlit", 			"../shaders/sprite_unlit_vs.fx.bin", 		"../shaders/model_unlit_ps.fx.bin");
		CHECK_SHADER("sprite_lit", 				"../shaders/sprite_lit_vs.fx.bin", 			"../shaders/model_lit_ps.fx.bin");
		CHECK_SHADER("sprite_distortion", 		"../shaders/sprite_distortion_vs.fx.bin", 	"../shaders/model_distortion_ps.fx.bin");
		CHECK_SHADER("sprite_adv_unlit", 		"../shaders/ad_sprite_unlit_vs.fx.bin", 	"../shaders/ad_model_unlit_ps.fx.bin");
		CHECK_SHADER("sprite_adv_lit", 			"../shaders/ad_sprite_lit_vs.fx.bin", 		"../shaders/ad_model_lit_ps.fx.bin");
		CHECK_SHADER("sprite_adv_distortion", 	"../shaders/ad_sprite_distortion_vs.fx.bin","../shaders/ad_model_distortion_ps.fx.bin");

		CHECK_SHADER("model_unlit", 			"../shaders/model_unlit_vs.fx.bin", 		"../shaders/model_unlit_ps.fx.bin");
		CHECK_SHADER("model_lit", 				"../shaders/model_lit_vs.fx.bin", 			"../shaders/model_lit_ps.fx.bin");
		CHECK_SHADER("model_distortion", 		"../shaders/model_distortion_vs.fx.bin", 	"../shaders/model_distortion_ps.fx.bin");
		CHECK_SHADER("model_adv_unlit", 		"../shaders/ad_model_unlit_vs.fx.bin", 		"../shaders/ad_model_unlit_ps.fx.bin");
		CHECK_SHADER("model_adv_lit", 			"../shaders/ad_model_lit_vs.fx.bin", 		"../shaders/ad_model_lit_ps.fx.bin");
		CHECK_SHADER("model_adv_distortion", 	"../shaders/ad_model_distortion_vs.fx.bin", "../shaders/ad_model_distortion_ps.fx.bin");

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

	static bgfx::TextureHandle
	loadPng(const char* filename, uint64_t state){
		const bgfx::Memory* mem = loadMem(entry::getFileReader(), filename);
		auto image = bimg::imageParse(entry::getAllocator(), mem->data, mem->size, bimg::TextureFormat::Enum(bgfx::TextureFormat::Count), nullptr);
		assert(image && "invalid png file");
		bimg::ImageContainer *dstimage = nullptr;
		if (image->m_format == bgfx::TextureFormat::RG8){
			dstimage = bimg::imageAlloc(entry::getAllocator()
				, bimg::TextureFormat::RGBA8
				, uint16_t(image->m_width)
				, uint16_t(image->m_height)
				, uint16_t(image->m_depth)
				, image->m_numLayers
				, image->m_cubeMap
				, false
			);
			auto unpack = [](float* dst, const void* src){
				const uint8_t* _src = (const uint8_t*)src;
				dst[0] = dst[1] = dst[2] = bx::fromUnorm(_src[0], 255.0f);
				if (_src[1] != 255 && _src[1] != 0){
					int debug = 0;
				}
				dst[3] = bx::fromUnorm(_src[1], 255.0f);
			};
			const auto srcbpp = 16;
			const auto dstbpp = 32;
			bimg::imageConvert(dstimage->m_data, dstbpp, bx::packRgba8, 
								image->m_data, srcbpp, unpack, 
								image->m_width, image->m_height, image->m_depth, 
								image->m_width * (srcbpp/8), image->m_width * (dstbpp/8));
		} else {
			dstimage = bimg::imageConvert(entry::getAllocator(), bimg::TextureFormat::RGBA8, *image, false);
		}

		imageFree(image);
		
		auto h = bgfx::createTexture2D(
				(uint16_t)dstimage->m_width
			, (uint16_t)dstimage->m_height
			, false
			, 1
			, bgfx::TextureFormat::BGRA8
			, state
			, bgfx::copy(dstimage->m_data, dstimage->m_size)
			);
		imageFree(dstimage);
		return h;
	}

	static bgfx::TextureHandle createTexture(const char* filename, uint64_t state){
		if (isPngFile(filename)){
			return loadPng(filename, state);
		}

		return bgfx::createTexture(loadMem(entry::getFileReader(), filename), state);
	}

	static bgfx_texture_handle_t TextureGet(int texture_type, void *parm, void *ud){
		EffekseerBgfxTest *that = (EffekseerBgfxTest *)ud;
		if (texture_type == TEXTURE_BACKGROUND)
			return {that->m_sceneTex.idx};

		if (texture_type == TEXTURE_DEPTH){
			EffekseerRenderer::DepthReconstructionParameter *p = (EffekseerRenderer::DepthReconstructionParameter*)parm;
			p->DepthBufferScale = 1.0f;
			p->DepthBufferOffset = 0.0f;

			p->ProjectionMatrix33 = that->m_projMat.Values[2][2];
			p->ProjectionMatrix34 = that->m_projMat.Values[2][3];
			p->ProjectionMatrix43 = that->m_projMat.Values[3][2];
			p->ProjectionMatrix44 = that->m_projMat.Values[4][4];
			return {that->m_sceneDepth.idx};
		}
		assert(false && "invalid texture type");
		return {BGFX_INVALID_HANDLE};
	}

	static bgfx_texture_handle_t TextureLoad(const char *name, int srgb, void *ud){
		const uint64_t state = (srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE)|BGFX_SAMPLER_NONE;
		auto handle = createTexture(name, state);
		bgfx::setName(handle, name);
		return {handle.idx};
	}

	static void TextureUnload(bgfx_texture_handle_t handle, void *ud){
		bgfx::destroy(bgfx::TextureHandle{handle.idx});
	}
private:
	EffekseerRenderer::RendererRef m_efkRenderer = nullptr;
	Effekseer::ManagerRef m_efkManager = nullptr;
	Effekseer::EffectRef m_efkEffect = nullptr;
	Effekseer::Handle m_efkEffectHandle = 0;
	Effekseer::Matrix44	m_projMat;
	Effekseer::Matrix44	m_viewMat;

	bgfx::TextureHandle m_sceneTex = BGFX_INVALID_HANDLE;
	bgfx::TextureHandle m_sceneDepth = BGFX_INVALID_HANDLE;
	bgfx::FrameBufferHandle m_frameBuffer = BGFX_INVALID_HANDLE;

	bgfx::VertexLayout m_cubeLayout;
	bgfx::VertexBufferHandle m_cubeVertexBuffer;
	bgfx::IndexBufferHandle m_cubeIndexBuffer;

	bgfx::ProgramHandle m_fullscreenProg;
	bgfx::ProgramHandle m_cubeProg;

	bgfx::UniformHandle m_fullscreenTextureUniformHandle;

	const uint64_t m_renderstate = 0
		| BGFX_STATE_WRITE_R
		| BGFX_STATE_WRITE_G
		| BGFX_STATE_WRITE_B
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CW
		| BGFX_STATE_MSAA;

	const uint64_t m_fullscreenstate = 	0
		| BGFX_STATE_WRITE_R
		| BGFX_STATE_WRITE_G
		| BGFX_STATE_WRITE_B
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_MSAA;
};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	  EffekseerBgfxTest
	, "EffekseerBgfxTest"
	, "Effekseer bgfx renderer example."
	, "https://github.com/cloudwu/efkbgfx"
	);
