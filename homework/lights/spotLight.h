#include <iostream>
#include <bx/math.h>



double radians(double degree) {
	double pi = 3.14159265359;
	return (degree * (pi / 180));
}

class SpotLight {
public:
	bx::Vec3 m_position;
	bx::Vec3 m_direction;
	bx::Vec3 m_color;

	float m_inPhi;
	float m_outPhi;

	float m_intensity;
	bool m_disable;

private:
	float m_inCutOff;
	float m_outCutOff;

	void computeCutOff() {
		m_inCutOff = bx::cos(radians(m_inPhi));
		m_outCutOff = bx::cos(radians(m_outPhi));
	}

public:
	//ctors
	SpotLight() : //default setting
		m_position(bx::Vec3(-1.0, 1.0, 1.0)), m_direction(bx::sub(bx::Vec3(0.0), bx::Vec3(-1.0, 1.0, 1.0))),
		m_color(bx::Vec3(1.0, 1.0, 1.0)),
		m_inPhi(12.5),m_outPhi(17.5),
		m_intensity(1.0),
		m_disable(false){
		computeCutOff();
	}

	SpotLight(bx::Vec3 position, bx::Vec3 direction, bx::Vec3 color, float inPhi, float outPhi, float intensity, bool disable) :
		m_position(position), m_direction(direction),
		m_color(color),
		m_inPhi(inPhi), m_outPhi(outPhi),
		m_intensity(intensity),
		m_disable(disable) {
		computeCutOff();
	}

	SpotLight(
		float posX, float posY, float posZ,
		float directX, float directY, float directZ,
		float colorR, float colorG, float colorB,
		float inPhi, float outPhi, float intensity, bool disable
	) :
		m_position(bx::Vec3(posX, posY, posZ)), m_direction(bx::Vec3(directX, directY, directZ)),
		m_color(bx::Vec3(colorR, colorG, colorB)),
		m_inPhi(inPhi), m_outPhi(outPhi),
		m_intensity(intensity),
		m_disable(disable) {
		computeCutOff();
	}


};
