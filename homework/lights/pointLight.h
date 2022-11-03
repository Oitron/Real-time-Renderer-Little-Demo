#include <iostream>
#include <bx/math.h>



class PointLight {
public:
	bx::Vec3 m_position;
	bx::Vec3 m_color;
	float m_intensity;
	bool m_disable;

	//ctors
	PointLight() : //default settings
		m_position(bx::Vec3(2.0, 2.0, 2.0)), m_color(bx::Vec3(1.0, 1.0, 1.0)),
		m_intensity(1.0),
		m_disable(false)
	{}

	PointLight(bx::Vec3 position, bx::Vec3 color, float intensity, bool disable) :
		m_position(position), m_color(color),
		m_intensity(intensity),
		m_disable(disable)
	{}

	PointLight(float posX, float posY, float posZ, float colorR, float colorG, float colorB, float intensity, bool disable) :
		m_position(bx::Vec3(posX, posY, posZ)), m_color(bx::Vec3(colorR, colorG, colorB)),
		m_intensity(intensity),
		m_disable(disable)
	{}
	

};