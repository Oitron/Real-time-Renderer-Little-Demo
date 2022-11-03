#include <iostream>
#include <bx/math.h>


class DirectionalLight {
public:
	bx::Vec3 m_direction;
	bx::Vec3 m_color;
	float m_intensity;
	bool m_disable;

	//ctors
	DirectionalLight() : //default setting
		m_direction(bx::Vec3(-1.0, -1.0, -1.0)), m_color(bx::Vec3(1.0, 1.0, 1.0)),
		m_intensity(1.0),
		m_disable(false)
	{}

	DirectionalLight(bx::Vec3 direction, bx::Vec3 color, float intensity, bool disable) :
		m_direction(direction), m_color(color),
		m_intensity(intensity),
		m_disable(disable)
	{}

	DirectionalLight(float directX, float directY, float directZ, float colorR, float colorG, float colorB, float intensity, bool disable) :
		m_direction(bx::Vec3(directX, directY, directZ)), m_color(bx::Vec3(colorR, colorG, colorB)),
		m_intensity(intensity),
		m_disable(disable)
	{}
};