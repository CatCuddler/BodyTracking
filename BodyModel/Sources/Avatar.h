#pragma once

#include "MeshObject.h"
#include "InverseKinematics.h"

class Avatar : public MeshObject {
	
private:
	InverseKinematics* invKin;
	float currentHeight;
	
public:
	Avatar(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f);
	
	void animate(Kore::Graphics4::TextureUnit tex);
	void setDesiredPositionAndOrientation(int boneIndex, IKMode ikMode, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	void setFixedPositionAndOrientation(int boneIndex, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	
	BoneNode* getBoneWithIndex(int index) const;
	
	void resetPositionAndRotation();
	
	float getReached() const;
	float getStucked() const;
	float* getIterations() const;
	float* getErrorPos() const;
	float* getErrorRot() const;
	float* getTime() const;
	float* getTimeIteration() const;
	
	float getHeight() const;
};
