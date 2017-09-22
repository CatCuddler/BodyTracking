#pragma once

#include "pch.h"
#include "MeshObject.h"

class LivingRoom : public MeshObject {

private:
	
public:
	LivingRoom(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f) : MeshObject(meshFile, textureFile, structure, scale) {
		
	}
	
};
