#pragma once

#include "pch.h"
#include "MeshObject.h"
#include "InverseKinematics.h"
#include "MoCap.h"

#include "Kore/Graphics2/Graphics.h"

class Avatar : public MeshObject {
	
private:
	// Inverse Kinematics
	InverseKinematics* invKin;
	Kore::vec4 desiredPosition;
	bool useIK;
	
	// MoCap
	MoCap* mocap;
	
	float currentHeight;
	
	void updateBone(BoneNode* bone);
	
	Kore::Graphics2::Graphics2* g2;
	Kore::Graphics4::Texture* redDot;
	Kore::Graphics4::Texture* yellowDot;
	
public:
	Avatar(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, bool useIK, float scale = 1.0f);
	
	void animate(Kore::Graphics4::TextureUnit tex, float deltaTime);
	
	void drawVertices(const Kore::mat4& modelMatrix, const Kore::mat4& viewMatrix, const Kore::mat4& projectionMatrix, int screenWidth, int screenHeight);
	void drawJoints(const Kore::mat4& modelMatrix, const Kore::mat4& viewMatrix, const Kore::mat4& projectionMatrix, int screenWidth, int screenHeight, bool skeleton);
	
	Kore::vec3 getBonePosition(int boneIndex) const;
	Kore::Quaternion getBoneLocalRotation(int boneIndex) const;
	Kore::Quaternion getBoneGlobalRotation(int boneIndex) const;
	
    int getJointDOFs(int boneIndex);
	void setDesiredPositionAndOrientation(int boneIndex, Kore::vec3 desiredPos, Kore::Quaternion desiredRot, int jointDOFs, bool posAndOrientation = true);
	
	BoneNode* getBoneWithIndex(int index) const;
	
	void setLocalRotation(int boneIndex, Kore::Quaternion desiredRotation);
	void setLocalRotation(int boneIndex, Kore::vec3 rot);
	
	float getAverageIKiterationNum() const;
	float getHeight() const;
	
	// Mocap
	void getNextMocapSet(Kore::vec3* rawPos);
};
