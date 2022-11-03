#pragma once

#include <bx/math.h>
#include "entry/entry.h"
#include "entry/cmd.h"
#include "entry/input.h"


#define CAMERA_KEY_LEFT      UINT8_C(0x01)
#define CAMERA_KEY_RIGHT     UINT8_C(0x02)
#define CAMERA_KEY_UP        UINT8_C(0x04)
#define CAMERA_KEY_DOWN      UINT8_C(0x08)

void CamCreate();
void CamDestroy();

void CamSetMouseProp(float& x_offset, float& y_offset, float& scroll_offset,
	int32_t curCursorX, int32_t curCursorY, int32_t curScrollVal,
	uint32_t screenWidth, uint32_t screenHeight);

void CamSetKeyState(uint8_t key, bool down);

void CamGetViewMatrix(float* _view_matrix);

void CamSwayMovement(float offsetX, float offsetY);

void CamOrbitMovement(float offsetX, float offsetY);

void CamFowBakMovement(float scrollOffset);

void CamWASDMovement(float deltaTime);

bool CamGetScrolling();

float CamGetNearPlane();
float CamGetFarPlane();

bx::Vec3 CamGetCamPos();