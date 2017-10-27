#pragma once

#include "pch.h"
#include "MeshObject.h"

class LivingRoom : public MeshObject {

private:
	
public:
	LivingRoom(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f);
	
	void render(Kore::Graphics4::TextureUnit tex, Kore::Graphics4::ConstantLocation mLocation, Kore::Graphics4::ConstantLocation mLocationInverse, Kore::Graphics4::ConstantLocation diffuseLocation, Kore::Graphics4::ConstantLocation specularLocation, Kore::Graphics4::ConstantLocation specularPowerLocation);
	
	void setLights(Kore::Graphics4::ConstantLocation lightCountLocation, Kore::Graphics4::ConstantLocation lightPosLocation);
	
private:
	static const int maxLightCount = 10;
	Kore::vec4 lightPositions[maxLightCount];
};
