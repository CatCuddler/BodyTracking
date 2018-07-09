#pragma once

#include "pch.h"
#include "MeshObject.h"
#include "InverseKinematics.h"

class Avatar : public MeshObject {
	
private:
	InverseKinematics* invKin;
	float currentHeight;
	
	void updateBone(BoneNode* bone);
	
public:
	Avatar(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f);
	
	void animate(Kore::Graphics4::TextureUnit tex, float deltaTime);
	void setDesiredPositionAndOrientation(int boneIndex, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	void setPosition(int boneIndex, Kore::vec3 desPosition);
	void setOrientation(int boneIndex, Kore::Quaternion desRotation);
	
	BoneNode* getBoneWithIndex(int index) const;
	
	int getTotalNum() const;
	float getAverageIkIteration() const;
	float getAverageIkReached() const;
	float getAverageIkError() const;
	float getMinIkError() const;
	float getMaxIkError() const;
	float getHeight() const;
};
