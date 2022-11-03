#include <bx/math.h>
#include "my_camera.h"
#include "entry/entry.h"
#include "entry/cmd.h"
#include "entry/input.h"

#include <iostream>


int MovementCmd(CmdContext* /*_context*/, void* /*_userData*/, int _argc, char const* const* _argv)
{
	if (_argc > 1)
	{
		if (0 == bx::strCmp(_argv[1], "left"))
		{
			CamSetKeyState(CAMERA_KEY_LEFT, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "right"))
		{
			CamSetKeyState(CAMERA_KEY_RIGHT, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "up"))
		{
			CamSetKeyState(CAMERA_KEY_UP, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "down"))
		{
			CamSetKeyState(CAMERA_KEY_DOWN, true);
			return 0;
		}
	}
	return 1;
}


static void cmd(const void* _userData)
{
	cmdExec((const char*)_userData);
}

static const InputBinding s_camBindings[] =
{
	{ entry::Key::KeyA,  entry::Modifier::None, 0, cmd, "move left"     },
	{ entry::Key::Left,  entry::Modifier::None, 0, cmd, "move left"     },
	{ entry::Key::KeyD,  entry::Modifier::None, 0, cmd, "move right"    },
	{ entry::Key::Right, entry::Modifier::None, 0, cmd, "move right"    },
	{ entry::Key::KeyS,  entry::Modifier::None, 0, cmd, "move down"     },
	{ entry::Key::Down,  entry::Modifier::None, 0, cmd, "move down"     },
	{ entry::Key::KeyW,  entry::Modifier::None, 0, cmd, "move up"       },
	{ entry::Key::Up,    entry::Modifier::None, 0, cmd, "move up"       },

	INPUT_BINDING_END
};




class Camera {

	// Needed Mouse values 
	struct MouseProp {
		//member vals
		int32_t m_preCursorX;
		int32_t m_preCursorY;
		int32_t m_preScrollVal;
		float m_cursorSen;
		float m_scrollSen;
		bool scrolling;
		//ctor
		MouseProp() :
			m_preScrollVal(0),
			m_preCursorX(0), m_preCursorY(0),
			m_cursorSen(2.0), m_scrollSen(1.0),
			scrolling(false) {};

		void GetOffset(float& x_offset, float& y_offset, float& scroll_offset,
			int32_t curCursorX, int32_t curCursorY, int32_t curScrollVal,
			uint32_t screenWidth, uint32_t screenHeight) {
			x_offset = m_cursorSen * (float)(curCursorX - m_preCursorX) / screenWidth;
			y_offset = m_cursorSen * (float)(curCursorY - m_preCursorY) / screenHeight;
			scroll_offset = m_scrollSen * (float)(curScrollVal - m_preScrollVal);
			//update values
			m_preCursorX = curCursorX;
			m_preCursorY = curCursorY;
			m_preScrollVal = curScrollVal;
			if (scroll_offset != 0) {
				scrolling = true;
			}
			else {
				scrolling = false;
			}
		}
	};


	//member vals
public:
	MouseProp m_mouseProp;
	bx::Vec3 m_camPos = bx::init::None;
	bx::Vec3 m_trgPos = bx::init::None;

	bx::Vec3 m_camDir = bx::init::None;
	bx::Vec3 m_camLeft = bx::init::None;
	bx::Vec3 m_camUp = bx::init::None;
	float m_distance;

	float m_nearPlane, m_farPlane;
	float m_orbitSpeed;
	float m_swaySpeed;
	float m_moveSpeed;
	float m_scrollSpeed;

	uint8_t m_keys;

	//ctors
	Camera() {
		m_trgPos = bx::init::Zero;
		m_camPos = bx::add(m_trgPos, bx::Vec3(0.0, 0.0, 5.0));
		m_nearPlane = 0.01;
		m_farPlane = 500.0;
		m_orbitSpeed = 30.0;
		m_swaySpeed = 7.0;
		m_moveSpeed = 7.0;
		m_scrollSpeed = 20.0;

		m_keys = 0;

		m_distance = bx::length(bx::sub(m_camPos, m_trgPos));
		CalDirLeftUp();

		cmdAdd("move", MovementCmd);
		inputAddBindings("camBindings", s_camBindings);
	}

	Camera(bx::Vec3& _camPos, bx::Vec3& _trgPos,
		float _nearPlane, float _farPlane,
		float _orbitSpeed, float _swaySpeed, float _moveSpeed, float _scrollSpeed)

		: m_camPos(_camPos), m_trgPos(_trgPos),
		m_nearPlane(_nearPlane), m_farPlane(_farPlane),
		m_orbitSpeed(_orbitSpeed), m_swaySpeed(_swaySpeed), m_moveSpeed(_moveSpeed), m_scrollSpeed(_scrollSpeed) {

		m_distance = bx::length(bx::sub(m_camPos, m_trgPos));
		CalDirLeftUp();

		cmdAdd("move", MovementCmd);
		inputAddBindings("camBindings", s_camBindings);
	}
	//dtor
	~Camera()
	{
		cmdRemove("move");
		inputRemoveBindings("camBindings");
	}

	bx::Vec3 Normalize(bx::Vec3 vector) {
		float length = bx::length(vector);
		float lengthInv = 1.0 / (length + bx::kFloatMin);
		return bx::mul(vector, lengthInv);
	}

	void CalDirLeftUp() {
		m_camDir = Normalize(bx::sub(m_trgPos, m_camPos));

		bx::Vec3 worldUp(0.0, 1.0, 0.0);
		m_camLeft = bx::cross(worldUp, m_camDir);
		m_camUp = bx::cross(m_camDir, m_camLeft);
	}

	void SwayMovement(float offsetX, float offsetY) {
		//convert to polar coords
		float phi = 0;
		float theta = 0;
		bx::toLatLong(&phi, &theta, m_camDir);
		phi -= offsetX * m_swaySpeed;
		theta -= offsetY * m_swaySpeed;
		//avoid flip
		theta = bx::clamp(theta, 0.01f, 0.99f);
		bx::Vec3 newDirection = bx::fromLatLong(phi, theta);
		bx::Vec3 offsetVector = bx::mul(bx::sub(newDirection, m_camDir), m_distance);
		m_trgPos = bx::add(m_trgPos, offsetVector);
		CalDirLeftUp();
	}


	void OrbitMovement(float offsetX, float offsetY) {
		//convert to polar coords
		float phi = 0;
		float theta = 0;
		bx::toLatLong(&phi, &theta, bx::mul(m_camDir, -1));
		phi += offsetX * m_orbitSpeed;
		theta -= offsetY * m_orbitSpeed;
		//avoid flip
		theta = bx::clamp(theta, 0.01f, 0.99f);
		bx::Vec3 newDirection = bx::fromLatLong(phi, theta);
		bx::Vec3 offsetVector = bx::mul(bx::sub(newDirection, bx::mul(m_camDir, -1)), m_distance);
		m_camPos = bx::add(m_camPos, offsetVector);
		CalDirLeftUp();
	}

	void FowBakMovement(float offsetScroll) {
		m_distance -= offsetScroll * m_scrollSpeed;
		m_distance = bx::clamp(m_distance, m_nearPlane, m_farPlane);
		bx::Vec3 newDirection = bx::mul(bx::mul(m_camDir, -1), m_distance);
		m_camPos = bx::add(m_trgPos, newDirection);
		//CalDirLeftUp();
	}

	void SetKeyState(uint8_t key, bool down){
		m_keys &= ~key;
		m_keys |= down ? key : 0;
	}

	void WASDMovement(float deltaTime) {

		if (m_keys & CAMERA_KEY_UP) {
			std::cout << "up down" << std::endl;
			m_camPos = bx::mad(m_camUp, deltaTime * m_moveSpeed, m_camPos);
			m_trgPos = bx::mad(m_camDir, m_distance, m_camPos);
			SetKeyState(CAMERA_KEY_UP, false);
		}
		if (m_keys & CAMERA_KEY_DOWN) {
			std::cout << "down down" << std::endl;
			m_camPos = bx::mad(m_camUp, deltaTime * -m_moveSpeed, m_camPos);
			m_trgPos = bx::mad(m_camDir, m_distance, m_camPos);
			SetKeyState(CAMERA_KEY_DOWN, false);
		}
		if (m_keys & CAMERA_KEY_LEFT) {
			std::cout << "left down" << std::endl;
			m_camPos = bx::mad(m_camLeft, deltaTime * -m_moveSpeed, m_camPos);
			m_trgPos = bx::mad(m_camDir, m_distance, m_camPos);
			SetKeyState(CAMERA_KEY_LEFT, false);
		}
		if (m_keys & CAMERA_KEY_RIGHT) {
			std::cout << "right down" << std::endl;
			m_camPos = bx::mad(m_camLeft, deltaTime * m_moveSpeed, m_camPos);
			m_trgPos = bx::mad(m_camDir, m_distance, m_camPos);
			SetKeyState(CAMERA_KEY_RIGHT, false);
		}
	}

	//gtor
	void GetViewMatrix(float* _view_matrix) {
		bx::mtxLookAt(_view_matrix, m_camPos, m_trgPos);
	}

	bx::Vec3 GetCamPos() {
		return m_camPos;
	}
};



static Camera* s_cam = NULL;

void CamCreate() {
	s_cam = BX_NEW(entry::getAllocator(), Camera);
}

void CamDestroy() {
	BX_DELETE(entry::getAllocator(), s_cam);
	s_cam = NULL;
}

void CamSetMouseProp(float& x_offset, float& y_offset, float& scroll_offset,
	int32_t curCursorX, int32_t curCursorY, int32_t curScrollVal,
	uint32_t screenWidth, uint32_t screenHeight) {

	s_cam->m_mouseProp.GetOffset(x_offset, y_offset, scroll_offset,
		curCursorX, curCursorY, curScrollVal,
		screenWidth, screenHeight);
}

void CamSetKeyState(uint8_t key, bool down) {
	s_cam->SetKeyState(key, down);
}

void CamGetViewMatrix(float* _view_matrix) {
	s_cam->GetViewMatrix(_view_matrix);
}

void CamSwayMovement(float offsetX, float offsetY) {
	s_cam->SwayMovement(offsetX, offsetY);
}

void CamOrbitMovement(float offsetX, float offsetY) {
	s_cam->OrbitMovement(offsetX, offsetY);
}

void CamFowBakMovement(float scrollOffset) {
	s_cam->FowBakMovement(scrollOffset);
}

void CamWASDMovement(float deltaTime) {
	s_cam->WASDMovement(deltaTime);
}

bool CamGetScrolling() {
	return s_cam->m_mouseProp.scrolling;
}

float CamGetNearPlane() {
	return s_cam->m_nearPlane;
}

float CamGetFarPlane() {
	return s_cam->m_farPlane;
}

bx::Vec3 CamGetCamPos() {
	return s_cam->GetCamPos();
}

