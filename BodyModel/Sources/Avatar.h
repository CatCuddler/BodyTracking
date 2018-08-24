#pragma once

#include "MeshObject.h"
#include "InverseKinematics.h"

class Avatar : public MeshObject {
	
private:
	InverseKinematics* invKin;
	float currentHeight;
	
public:
	Avatar(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f);
	
	void animate(Kore::Graphics4::TextureUnit tex, float deltaTime);
	void setDesiredPositionAndOrientation(int boneIndex, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	void setFixedPositionAndOrientation(int boneIndex, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	
	Kore::Quaternion getLocalCoordinateSystem(int boneIndex);
	
	BoneNode* getBoneWithIndex(int index) const;
	
	float getReached() const;
	float getStucked() const;
	float* getIterations() const;
	float* getErrorPos() const;
	float* getErrorRot() const;
	float* getTime() const;
	float* getTimeIteration() const;
	
	float getHeight() const;
};
