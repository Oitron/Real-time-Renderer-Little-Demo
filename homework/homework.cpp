/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <iostream>
#include <bx/uint32_t.h>
#include "common.h"

#include "my_camera.h"
#include "lights/pointLight.h"

#include "bgfx_utils.h"
#include "bgfx_logo.h"
#include "imgui/imgui.h"

#include "entry/entry.h"
#include "entry/cmd.h"
#include "entry/input.h"

namespace
{

#define RENDER_BRDF_ID   0
#define RENDER_SKYBOX_ID 1
#define RENDER_SHADOW_ID 2
#define RENDER_SCENE_ID  3




#define SHADOW_MAP_WIDTH  1024
#define SHADOW_MAP_HEIGHT 1024


#define BRDF_LUT_SIZE 512






struct PosVertex {
	float m_x;
	float m_y;
	float m_z;

	static bgfx::VertexLayout ms_layout;
	static void init() {
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.end();
	};
};

bgfx::VertexLayout PosVertex::ms_layout;

struct PosTexVertex {
	float m_x;
	float m_y;
	float m_z;

	float m_tx;
	float m_ty;

	static bgfx::VertexLayout ms_layout;
	static void init() {
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	};
};

bgfx::VertexLayout PosTexVertex::ms_layout;


static PosVertex s_skyBoxVertices[] = {
	{-1.0,  1.0, -1.0},
	{-1.0, -1.0, -1.0},
	{ 1.0, -1.0, -1.0},
	{ 1.0, -1.0, -1.0},
	{ 1.0,  1.0, -1.0},
	{-1.0,  1.0, -1.0},

	{-1.0, -1.0,  1.0},
	{-1.0, -1.0, -1.0},
	{-1.0,  1.0, -1.0},
	{-1.0,  1.0, -1.0},
	{-1.0,  1.0,  1.0},
	{-1.0, -1.0,  1.0},

	{ 1.0, -1.0, -1.0},
	{ 1.0, -1.0,  1.0},
	{ 1.0,  1.0,  1.0},
	{ 1.0,  1.0,  1.0},
	{ 1.0,  1.0, -1.0},
	{ 1.0, -1.0, -1.0},

	{-1.0, -1.0,  1.0},
	{-1.0,  1.0,  1.0},
	{ 1.0,  1.0,  1.0},
	{ 1.0,  1.0,  1.0},
	{ 1.0, -1.0,  1.0},
	{-1.0, -1.0,  1.0},

	{-1.0,  1.0, -1.0},
	{ 1.0,  1.0, -1.0},
	{ 1.0,  1.0,  1.0},
	{ 1.0,  1.0,  1.0},
	{-1.0,  1.0,  1.0},
	{-1.0,  1.0, -1.0},

	{-1.0, -1.0, -1.0},
	{-1.0, -1.0,  1.0},
	{ 1.0, -1.0, -1.0},
	{ 1.0, -1.0, -1.0},
	{-1.0, -1.0,  1.0},
	{ 1.0, -1.0,  1.0},
};

static PosTexVertex quadVertices[] = {
	{-1.0,  1.0, 0.0, 0.0, 1.0}, //1
	{-1.0, -1.0, 0.0, 0.0, 0.0}, //2
	{ 1.0,  1.0, 0.0, 1.0, 1.0}, //3

	{ 1.0, -1.0, 0.0, 1.0, 0.0}, //4
	{ 1.0,  1.0, 0.0, 1.0, 1.0}, //3
	{-1.0, -1.0, 0.0, 0.0, 0.0}, //2
};


void renderQuad(bgfx::ProgramHandle& program, bgfx::ViewId id) {
	bgfx::VertexBufferHandle quadVbh;
	PosTexVertex::init();
	quadVbh = bgfx::createVertexBuffer(
		//static data can be passed with bgfx::makeRef
		bgfx::makeRef(quadVertices, sizeof(quadVertices)),
		PosTexVertex::ms_layout
	);
	bgfx::setState(id
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
	);
	bgfx::setVertexBuffer(0, quadVbh);
	bgfx::submit(id, program);
	std::cout << "render brdf!!!" << std::endl;
}




class EStarHomework : public entry::AppI
{
public:

	entry::MouseState m_mouseState;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

	bgfx::VertexBufferHandle m_skyBoxVbh;

	Mesh* m_stoneMesh;
	Mesh* m_orbMesh;
	Mesh* m_platMesh;
	Mesh* m_lightMesh;

	bgfx::ProgramHandle m_lightProgram;
	bgfx::ProgramHandle m_phongProgram;
	bgfx::ProgramHandle m_phongNontProgram;
	bgfx::ProgramHandle m_pbrProgram;
	bgfx::ProgramHandle m_pbrNontProgram;
	bgfx::ProgramHandle m_skyBoxProgram;

	bgfx::ProgramHandle m_brdfProgram;

	bgfx::ProgramHandle m_shadowMapProgram;
	

	bgfx::UniformHandle u_objColor; //for no texture object
	bgfx::UniformHandle u_objRM; //for no texture object

	bgfx::UniformHandle u_lightPos;
	bgfx::UniformHandle u_lightColor;
	bgfx::UniformHandle u_viewPos;

	bgfx::UniformHandle u_phongSet;

	bgfx::UniformHandle u_ambientIntensity;

	bgfx::UniformHandle u_lightSpaceMatrix;


	bgfx::UniformHandle s_texColor;
	bgfx::UniformHandle s_texNormal;
	bgfx::UniformHandle s_texRM;

	bgfx::UniformHandle s_texEnvDiff;
	bgfx::UniformHandle s_texEnvSpec;

	bgfx::UniformHandle s_texBrdfLut;
	bgfx::UniformHandle s_texShadowMap;

	

	
	bgfx::TextureHandle m_texColor;
	bgfx::TextureHandle m_texNormal;
	bgfx::TextureHandle m_texRM;

	bgfx::TextureHandle m_texEnvDiff;
	bgfx::TextureHandle m_texEnvSpec;

	bgfx::TextureHandle m_texBrdfLut;
	bgfx::TextureHandle m_texShadowMap;

	bgfx::FrameBufferHandle m_fbBrdfLut;
	bgfx::FrameBufferHandle m_fbShadowMap;

	//imgui
	bool m_pbrOn;
	float m_setLightPos[3];
	float m_setLightColor[3];
	float m_setLightIntensity;
	float m_setAmbientIntensity;
	float m_setMetallic;
	float m_setRoughness;


	//ctor
	EStarHomework(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
		m_width = 0;
		m_height = 0;
		m_debug = BGFX_DEBUG_NONE;
		m_reset = BGFX_RESET_NONE;
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_TEXT;
		m_reset  = BGFX_RESET_VSYNC;


		bgfx::Init init;
		//init.type     = args.m_type;
		init.type = bgfx::RendererType::Enum::OpenGL;
		//init.type = bgfx::RendererType::Enum::Direct3D9;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		
		bgfx::setViewClear(RENDER_BRDF_ID
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
		);
		

		
		bgfx::setViewClear(RENDER_SKYBOX_ID
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
		);


		bgfx::setViewClear(RENDER_SHADOW_ID
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
		);
		
		/*
		bgfx::setViewClear(RENDER_SCENE_ID
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
		);
		*/
		

		//create vertex stream declaration
		PosVertex::init();
		//create static vertex buffer
		
		m_skyBoxVbh = bgfx::createVertexBuffer(
			//static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_skyBoxVertices, sizeof(s_skyBoxVertices)),
			PosVertex::ms_layout
		);

		
		//set uniform value
		u_objColor = bgfx::createUniform("u_objColor", bgfx::UniformType::Vec4);
		u_objRM = bgfx::createUniform("u_objRM", bgfx::UniformType::Vec4);
		u_lightPos = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4);
		u_lightColor = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4);
		u_viewPos = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);
		u_phongSet = bgfx::createUniform("u_phongSet", bgfx::UniformType::Vec4);
		u_ambientIntensity = bgfx::createUniform("u_ambientIntensity", bgfx::UniformType::Vec4);

		u_lightSpaceMatrix = bgfx::createUniform("u_lightSpaceMatrix", bgfx::UniformType::Mat4);

		//create texture sampler uniform
		s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
		s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
		s_texRM = bgfx::createUniform("s_texRM", bgfx::UniformType::Sampler);

		s_texEnvDiff = bgfx::createUniform("s_texEnvDiff", bgfx::UniformType::Sampler);
		s_texEnvSpec = bgfx::createUniform("s_texEnvSpec", bgfx::UniformType::Sampler);

		s_texBrdfLut = bgfx::createUniform("s_texBrdfLut", bgfx::UniformType::Sampler);
		
		s_texShadowMap = bgfx::createUniform("s_texShadowMap", bgfx::UniformType::Sampler);

		//create camera
		CamCreate();

		//create shader program
		m_lightProgram = loadProgram("vs_light", "fs_light");
		m_phongProgram = loadProgram("vs_phong", "fs_phong");
		m_phongNontProgram = loadProgram("vs_phong_nont", "fs_phong_nont");
		m_pbrProgram = loadProgram("vs_pbr", "fs_pbr");
		m_pbrNontProgram = loadProgram("vs_pbr_nont", "fs_pbr_nont");
		m_skyBoxProgram = loadProgram("vs_skybox", "fs_skybox");

		m_brdfProgram = loadProgram("vs_brdf", "fs_brdf");

		m_shadowMapProgram = loadProgram("vs_shadowMap", "fs_shadowMap");

		//create mesh
		//m_mesh = meshLoad("../resource/basic_meshes/bunny.bin");
		m_stoneMesh = meshLoad("../resource/pbr_stone/pbr_stone_nt.bin");
		m_orbMesh   = meshLoad("../resource/basic_meshes/bunny.bin");
		m_platMesh  = meshLoad("../resource/basic_meshes/platform.bin");
		m_lightMesh = meshLoad("../resource/basic_meshes/unit_sphere.bin");
		//m_mesh = meshLoad("../resource/nikon_camera/Camara_Analogica.bin");

		//load textures
		m_texColor = loadTexture("../resource/pbr_stone/pbr_stone_base_color.dds");
		m_texNormal = loadTexture("../resource/pbr_stone/pbr_stone_normal.dds");
		m_texRM = loadTexture("../resource/pbr_stone/pbr_stone_aorm.dds");

		m_texEnvDiff = loadTexture("../resource/env_maps/bolonga_irr.dds");
		m_texEnvSpec = loadTexture("../resource/env_maps/bolonga_lod.dds");


		m_texBrdfLut = bgfx::createTexture2D(
			512, 512,
			false,
			1,
			bgfx::TextureFormat::RG16F,
			BGFX_TEXTURE_RT //| 
			//BGFX_BUFFER_DRAW_INDIRECT |
			//BGFX_TEXTURE_COMPUTE_WRITE
		);

		m_fbBrdfLut = bgfx::createFrameBuffer(
			1,
			&m_texBrdfLut,
			true //why true not false
		);
		
		m_texShadowMap = bgfx::createTexture2D(
			SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,
			false,
			1,
			bgfx::TextureFormat::D32F,
			BGFX_TEXTURE_RT | 
			BGFX_SAMPLER_UVW_BORDER | 
			BGFX_CAPS_FRAGMENT_DEPTH |
			BGFX_SAMPLER_COMPARE_LEQUAL
		);

		m_fbShadowMap = bgfx::createFrameBuffer(
			1,
			&m_texShadowMap,
			true
		);

		
		

		bgfx::setViewRect(RENDER_BRDF_ID, 0, 0, BRDF_LUT_SIZE, BRDF_LUT_SIZE);
		bgfx::setViewFrameBuffer(RENDER_BRDF_ID, m_fbBrdfLut);
		renderQuad(m_brdfProgram, RENDER_BRDF_ID);


		//imgui
		m_pbrOn = false;
		m_setLightPos[0] = 3.0; m_setLightPos[1] = 5.0; m_setLightPos[2] = 3.0;
		m_setLightColor[0] = 1.0; m_setLightColor[1] = 1.0; m_setLightColor[2] = 1.0;
		m_setLightIntensity = 15.0;
		m_setAmbientIntensity = 0.2;
		m_setMetallic = 1.0;
		m_setRoughness = 0.2;
		
		imguiCreate();
	}


	virtual int shutdown() override
	{
		CamDestroy();

		imguiDestroy();

		meshUnload(m_stoneMesh);
		meshUnload(m_lightMesh);
		meshUnload(m_platMesh);
		meshUnload(m_orbMesh);
		
		//clean up
		bgfx::destroy(m_skyBoxVbh);

		
		bgfx::destroy(m_lightProgram);
		bgfx::destroy(m_phongProgram);
		bgfx::destroy(m_phongNontProgram);
		bgfx::destroy(m_pbrProgram);
		bgfx::destroy(m_pbrNontProgram);
		bgfx::destroy(m_skyBoxProgram);
		bgfx::destroy(m_brdfProgram);

		bgfx::destroy(m_shadowMapProgram);

		bgfx::destroy(m_texColor);
		bgfx::destroy(m_texNormal);
		bgfx::destroy(m_texRM);

		bgfx::destroy(m_texEnvDiff);
		bgfx::destroy(m_texEnvSpec);

		bgfx::destroy(m_texBrdfLut);
		bgfx::destroy(m_texShadowMap);


		bgfx::destroy(s_texColor);
		bgfx::destroy(s_texNormal);
		bgfx::destroy(s_texRM);

		bgfx::destroy(s_texEnvDiff);
		bgfx::destroy(s_texEnvSpec);

		bgfx::destroy(s_texBrdfLut);
		bgfx::destroy(s_texShadowMap);

		bgfx::destroy(m_fbBrdfLut);
		bgfx::destroy(m_fbShadowMap);


		bgfx::destroy(u_objColor);
		bgfx::destroy(u_objRM);
		bgfx::destroy(u_lightPos);
		bgfx::destroy(u_lightColor);
		bgfx::destroy(u_viewPos);
		bgfx::destroy(u_phongSet);
		bgfx::destroy(u_ambientIntensity);
		bgfx::destroy(u_lightSpaceMatrix);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			imguiBeginFrame(m_mouseState.m_mx
				,  m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				,  m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				);

			showExampleDialog(this);

			ImGui::Begin("settings: "
				, NULL
				, 0
			);
			ImGui::Text("PBR: ");
			if (ImGui::Button(m_pbrOn ? "ON" : "OFF"))
			{
				m_pbrOn = !m_pbrOn;
			}

			ImGui::SliderFloat3("light position: ", m_setLightPos, -20.0f, 20.0f);
			ImGui::ColorEdit3("light color: ", m_setLightColor);
			ImGui::SliderFloat("light intensity: ", &m_setLightIntensity, 0.0, 100.0);
			ImGui::SliderFloat("Ambient intensity: ", &m_setAmbientIntensity, 0.0, 1.0);
			ImGui::SliderFloat("metallic: ", &m_setMetallic, 0.0, 1.0);
			ImGui::SliderFloat("roughness: ", &m_setRoughness, 0.01, 1.0);

			ImGui::End();


			imguiEndFrame();


			// Set view 0 default viewport.
			//bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			//bgfx::touch(0);



			//update frame timer
			int64_t now = bx::getHPCounter();
			static int64_t last = now;
			const int64_t frameTime = now - last;
			last = now;
			const double freq = double(bx::getHPFrequency());
			const float deltaTime = float(frameTime / freq);

			float cursor_offset_x = 0.0;
			float cursor_offset_y = 0.0;
			float scroll_offset = 0.0;

			//update camera
			CamSetMouseProp(cursor_offset_x, cursor_offset_y, scroll_offset,
				m_mouseState.m_mx, m_mouseState.m_my, m_mouseState.m_mz,
				m_width, m_height);

			if (m_mouseState.m_buttons[entry::MouseButton::Left] && !ImGui::MouseOverArea()) {
				std::cout << "left mouse button down !" << std::endl;
				CamSwayMovement(cursor_offset_x * deltaTime, cursor_offset_y * deltaTime);
				std::cout << "x offset: " << cursor_offset_x << std::endl;
				std::cout << "y offset: " << cursor_offset_y << std::endl;
			}
			if (m_mouseState.m_buttons[entry::MouseButton::Right]) {
				std::cout << "right mouse button down !" << std::endl;
				CamOrbitMovement(cursor_offset_x * deltaTime, cursor_offset_y * deltaTime);
				std::cout << "x offset: " << cursor_offset_x << std::endl;
				std::cout << "y offset: " << cursor_offset_y << std::endl;
			}
			if (CamGetScrolling()) {
				std::cout << "scroll !" << std::endl;
				CamFowBakMovement(scroll_offset * deltaTime);
				std::cout << "scroll offset: " << scroll_offset << std::endl;
			}

			CamWASDMovement(deltaTime);


			float view[16];
			CamGetViewMatrix(view);
			//cameraGetViewMtx(view);
			float proj[16];
			bx::mtxProj(proj, 45.0f, float(m_width) / float(m_height),
				CamGetNearPlane(), CamGetFarPlane(),
				bgfx::getCaps()->homogeneousDepth);



			//value settings
			float platColor[4] = { 0.5, 0.5, 1.0, 1.0 };
			float platRM[4] = { 0.8, 0.2, 0.0, 0.0 };
			float orbColor[4] = { 0.5, 1.0, 0.5, 1.0 };
			float orbRM[4] = { m_setRoughness, m_setMetallic, 0.0, 0.0 };


			float lightPos[4] = { 
				m_setLightPos[0], 
				m_setLightPos[1], 
				m_setLightPos[2], 
				0.0 
			};
			bgfx::setUniform(u_lightPos, lightPos);

			float lightColor[4] = { 
				m_setLightColor[0] * m_setLightIntensity, 
				m_setLightColor[1] * m_setLightIntensity,
				m_setLightColor[2] * m_setLightIntensity,
				1.0 
			}; //with intensity include
			bgfx::setUniform(u_lightColor, lightColor);

			float ambientIntensity[4] = {
				m_setAmbientIntensity,
				m_setAmbientIntensity,
				m_setAmbientIntensity,
				1.0
			};
			bgfx::setUniform(u_ambientIntensity, ambientIntensity);


			bx::Vec3 camPosition = CamGetCamPos();
			float camPos[4] = { camPosition.x, camPosition.y, camPosition.z, 0.0 };
			bgfx::setUniform(u_viewPos, camPos);


			float phongStoneSet[4] = { 0.1, 1.0, 0.3, 32.0 }; //ka, kd, ks, shiniess
			float phongOrbSet[4] = { 0.1, 1.0, 0.3, 32.0 }; //ka, kd, ks, shiniess
			float phongPlatSet[4] = { 0.1, 1.0, 0.05, 128.0 }; //ka, kd, ks, shiniess

			
			
			bgfx::setViewClear(RENDER_SHADOW_ID
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x00FFFFFF
				, 1.0f
				, 0
			);
			
			

			/*
			bgfx::setViewClear(RENDER_SCENE_ID
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);
			*/
			
			
			
			
			





			//------------------first pass: render shadowmap---------------------------//

			
			float lightProjMtx[16];
			float lightViewMtx[16];
			float near_plane = 1.0, far_plane = 100.0;
			bx::mtxProj(
				lightProjMtx, 120.0, float(SHADOW_MAP_WIDTH)/float(SHADOW_MAP_HEIGHT), 
				near_plane, far_plane, 
				bgfx::getCaps()->homogeneousDepth);
			bx::Vec3 eye = { lightPos[0], lightPos[1], lightPos[2] };
			bx::Vec3 at = { 0.0, -3.0, 0.0 };
			bx::mtxLookAt(lightViewMtx, eye, at);

			
			

			


			
			


			//transform settings
			float lightModel[16];
			bx::mtxSRT(lightModel,
				0.2, 0.2, 0.2,							//scale
				0.0, 0.0, 0.0,							//rotate
				lightPos[0], lightPos[1], lightPos[2]	//translate
			);

			float modelSkyBox[16];
			bx::mtxSRT(modelSkyBox,
				400.0, 400.0, 400.0, //scale
				0.0, 0.0, 0.0,		 //rotate
				0.0, 0.0, 0.0		 //translate
			);

			//model matrix
			float stoneModel[16];
			bx::mtxSRT(stoneModel,
				1.0, 1.0, 1.0,	//scale
				0.0, 180.0, 0.0,	//rotate
				-2.0, -1.0, 0.0	//translate
			);
			//bgfx::setTransform(model);

			float platModel[16];
			bx::mtxSRT(platModel,
				10.0,  10.0, 10.0,	//scale
				0.0,   0.0,  0.0,	//rotate
				0.0,  -3.0,  0.0	//translate
			);

			float orbModel[16];
			bx::mtxSRT(orbModel,
				2.0, 2.0, 2.0,	//scale
				0.0, 30, 0.0,	//rotate
				3.0, -1.0, 0.0	//translate
			);



			bgfx::setViewRect(RENDER_SHADOW_ID, 0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
			bgfx::setViewFrameBuffer(RENDER_SHADOW_ID, m_fbShadowMap);
			
			
			float lightSpaceMatrix[16];
			//bx::mtxMul ???? b*a ?
			bx::mtxMul(lightSpaceMatrix, lightViewMtx, lightProjMtx);
			/*
			for (int i = 0; i < 16; ++i) {
				std::cout << lightSpaceMatrix[i] << ", ";
			}
			*/
			//std::cout << std::endl;
			
			bgfx::setUniform(u_lightSpaceMatrix, lightSpaceMatrix);
			bgfx::setState(RENDER_SHADOW_ID
				//| BGFX_STATE_WRITE_RGB
				//| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_MSAA
			);
			//bgfx::setViewTransform(RENDER_SHADOW_ID, lightViewMtx, lightProjMtx);
			meshSubmit(m_platMesh, RENDER_SHADOW_ID, m_shadowMapProgram, platModel);
			meshSubmit(m_stoneMesh, RENDER_SHADOW_ID, m_shadowMapProgram, stoneModel);
			meshSubmit(m_orbMesh, RENDER_SHADOW_ID, m_shadowMapProgram, orbModel);
			
			
			//std::cout << "shadow cao" << std::endl;
			//bgfx::frame();

			

			//------------------second pass: render scene---------------------------//

			//view rect
			bgfx::setViewRect(RENDER_SKYBOX_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
			bgfx::setViewRect(RENDER_SCENE_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));

			//submit view 0
			//render skybox
			bgfx::setTexture(0, s_texEnvSpec, m_texEnvSpec);
			bgfx::setState(RENDER_SKYBOX_ID 
				| BGFX_STATE_WRITE_RGB 
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_MSAA
			);
			bgfx::setVertexBuffer(0, m_skyBoxVbh);
			bgfx::setTransform(modelSkyBox);
			bgfx::setViewTransform(RENDER_SKYBOX_ID, view, proj);
			bgfx::submit(RENDER_SKYBOX_ID, m_skyBoxProgram);

			
			//submit view 1
			//render objs

			bgfx::setState(RENDER_SCENE_ID
				| BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_MSAA
			);

			
			bgfx::setViewTransform(RENDER_SCENE_ID, view, proj);



			setUniform(u_phongSet, phongStoneSet);
			setUniform(u_lightSpaceMatrix, lightSpaceMatrix);



			bgfx::setTexture(0, s_texEnvDiff, m_texEnvDiff);
			bgfx::setTexture(1, s_texEnvSpec, m_texEnvSpec);
			bgfx::setTexture(2, s_texBrdfLut, m_texBrdfLut);
			bgfx::setTexture(3, s_texShadowMap, m_texShadowMap);

			bgfx::setTexture(4, s_texColor, m_texColor);
			bgfx::setTexture(5, s_texNormal, m_texNormal);
			bgfx::setTexture(6, s_texRM, m_texRM);

			if (m_pbrOn) {
				meshSubmit(m_stoneMesh, RENDER_SCENE_ID, m_pbrProgram, stoneModel);
			}
			else {
				meshSubmit(m_stoneMesh, RENDER_SCENE_ID, m_phongProgram, stoneModel);
			}
			
			
			//orb rendering setting
			setUniform(u_lightSpaceMatrix, lightSpaceMatrix);
			setUniform(u_phongSet, phongOrbSet);
			setUniform(u_objColor, orbColor);
			setUniform(u_objRM, orbRM);
			bgfx::setTexture(0, s_texEnvDiff, m_texEnvDiff);
			bgfx::setTexture(1, s_texEnvSpec, m_texEnvSpec);
			bgfx::setTexture(2, s_texBrdfLut, m_texBrdfLut);
			bgfx::setTexture(3, s_texShadowMap, m_texShadowMap);
			if (m_pbrOn) {
				meshSubmit(m_orbMesh, RENDER_SCENE_ID, m_pbrNontProgram, orbModel);
			}
			else {
				meshSubmit(m_orbMesh, RENDER_SCENE_ID, m_phongNontProgram, orbModel);
			}
			
			//plat rendering setting
			setUniform(u_lightSpaceMatrix, lightSpaceMatrix);
			setUniform(u_phongSet, phongPlatSet);
			setUniform(u_objColor, platColor);
			setUniform(u_objRM, platRM);
			bgfx::setTexture(0, s_texEnvDiff, m_texEnvDiff);
			bgfx::setTexture(1, s_texEnvSpec, m_texEnvSpec);
			bgfx::setTexture(2, s_texBrdfLut, m_texBrdfLut);
			bgfx::setTexture(3, s_texShadowMap, m_texShadowMap);
			if (m_pbrOn) {
				meshSubmit(m_platMesh, RENDER_SCENE_ID, m_pbrNontProgram, platModel);
			}
			else {
				meshSubmit(m_platMesh, RENDER_SCENE_ID, m_phongNontProgram, platModel);
			}
			

			

			meshSubmit(m_lightMesh, RENDER_SCENE_ID, m_lightProgram, lightModel);

			

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	
};

} // namespace

int _main_(int _argc, char** _argv)
{
	EStarHomework app("e-star-homework", "", "");
	return entry::runApp(&app, _argc, _argv);
}

