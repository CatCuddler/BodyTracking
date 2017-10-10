#include "Avatar.h"

#include "RotationUtility.h"
#include "InverseKinematics.h"

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
	vec2 convert(vec4 pos, const mat4& modelMatrix, const mat4& viewMatrix, const mat4& projectionMatrix, int screenWidth, int screenHeight) {
		vec4 nPos = modelMatrix * pos;
		nPos = viewMatrix * nPos;
		nPos = projectionMatrix * nPos;
		
		nPos.x() = nPos.x() / nPos.z();
		nPos.y() = nPos.y() / nPos.z();
		nPos.x() = screenWidth * (nPos.x() + 1.0) / 2.0;
		nPos.y() = screenHeight * (1.0 - ((nPos.y() + 1.0) / 2.0));
		
		return vec2(nPos.x(), nPos.y());
	}
}

Avatar::Avatar(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale) : MeshObject(meshFile, textureFile, structure, scale) {
	
	// Update bones
	for (int i = 0; i < bones.size(); ++i) updateBone(bones[i]);
	
	//if (bones.size() > 0) {
	// Get the highest position
	BoneNode* head = getBoneWithIndex(46);
	Kore::vec4 position = head->combined * Kore::vec4(0, 0, 0, 1);
	position *= 1.0/position.w();
	currentHeight = position.z();
	
	invKin = new InverseKinematics(bones, 100);
	//}
	
	g2 = new Kore::Graphics2::Graphics2(1024, 768);
	redDot = new Kore::Graphics4::Texture("redDot.png");
	yellowDot = new Kore::Graphics4::Texture("yellowDot.png");
}

void Avatar::updateBone(BoneNode* bone) {
	bone->combined = bone->parent->combined * bone->local;
	
	if (!bone->initialized) {
		bone->initialized = true;
		bone->combinedInv = bone->combined.Invert();
	}
	
	bone->finalTransform = bone->combined * bone->combinedInv;
}

void Avatar::animate(TextureUnit tex, float deltaTime) {
	// Interpolate
	/*for (int i = 0; i < bones.size(); ++i) {
		BoneNode* bone = bones[i];
		
		if (bone->interQuat == Quaternion(0, 0, 0, 1)) bone->interQuat = bone->quaternion;
		
		Quaternion quatDiff = bone->quaternion.rotated(bone->interQuat.invert());
		vec3 rot;
		RotationUtility::quatToEuler(&quatDiff, &rot.x(), &rot.y(), &rot.z());
		
		if (rot.getLength() > 0.1) {
		//if (true) {
	 //log(Info, "interpolate %s %f", bone->boneName, rot.getLength());
		
	 bone->interQuat = bone->interQuat.slerp(0.1, bone->quaternion);
	 bone->quaternion = bone->interQuat;
	 
	 Kore::mat4 rotMat = bone->quaternion.matrix().Transpose();
	 bone->local = bone->transform * rotMat;
		
		} else {
	 //log(Info, "dont interpolate %s %f", bone->boneName, rot.getLength());
	 bone->interQuat = bone->quaternion;
		}
	 }*/
	
	// Update bones
	for (int i = 0; i < bones.size(); ++i) updateBone(bones[i]);
	
	for(int j = 0; j < meshesCount; ++j) {
		int currentBoneIndex = 0;	// Iterate over BoneCountArray
		
		Mesh* mesh = meshes[j];
		
		// Mesh Vertex Buffer
		float* vertices = vertexBuffers[j]->lock();
		for (int i = 0; i < mesh->numVertices; ++i) {
			vec4 startPos(0, 0, 0, 1);
			vec4 startNormal(0, 0, 0, 1);
			
			// For each vertex belonging to a mesh, the bone count array specifies the number of bones the influence the vertex
			int numOfBones = mesh->boneCountArray[i];
			
			float totalJointsWeight = 0;
			for (int b = 0; b < numOfBones; ++b) {
				vec4 posVec(mesh->vertices[i * 3 + 0], mesh->vertices[i * 3 + 1], mesh->vertices[i * 3 + 2], 1);
				vec4 norVec(mesh->normals[i * 3 + 0], mesh->normals[i * 3 + 1], mesh->normals[i * 3 + 2], 1);
				
				int index = mesh->boneIndices[currentBoneIndex] + 2;
				//BoneNode* bone = bones[mesh->boneIndices[currentBoneIndex] + 1];
				BoneNode* bone = getBoneWithIndex(index);
				float boneWeight = mesh->boneWeight[currentBoneIndex];
				totalJointsWeight += boneWeight;
				
				startPos += (bone->finalTransform * posVec) * boneWeight;
				startNormal += (bone->finalTransform * norVec) * boneWeight;
				
				currentBoneIndex ++;
			}
			
			// position
			vertices[i * 8 + 0] = startPos.x() * scale;
			vertices[i * 8 + 1] = startPos.y() * scale;
			vertices[i * 8 + 2] = startPos.z() * scale;
			// texCoord
			vertices[i * 8 + 3] = mesh->texcoord[i * 2 + 0];
			vertices[i * 8 + 4] = 1.0f - mesh->texcoord[i * 2 + 1];
			// normal
			vertices[i * 8 + 5] = startNormal.x();
			vertices[i * 8 + 6] = startNormal.y();
			vertices[i * 8 + 7] = startNormal.z();
			
			//log(Info, "%f %f %f %f %f %f %f %f", vertices[i * 8 + 0], vertices[i * 8 + 1], vertices[i * 8 + 2], vertices[i * 8 + 3], vertices[i * 8 + 4], vertices[i * 8 + 5], vertices[i * 8 + 6], vertices[i * 8 + 7]);
		}
		vertexBuffers[j]->unlock();
		
		Texture* image = images[j];
		
		Graphics4::setTexture(tex, image);
		Graphics4::setVertexBuffer(*vertexBuffers[j]);
		Graphics4::setIndexBuffer(*indexBuffers[j]);
		Graphics4::drawIndexedVertices();
	}
}

void Avatar::drawVertices(const mat4& modelMatrix, const Kore::mat4& viewMatrix, const Kore::mat4& projectionMatrix, int screenWidth, int screenHeight) {
	g2->begin(false);
	
	for (int i = 0; i < meshesCount; ++i) {
		Mesh* mesh = meshes[i];
		for (int i2 = 0; i2 < mesh->numVertices; ++i2) {
			vec4 pos = vec4(mesh->vertices[i2 * 3 + 0] * scale, mesh->vertices[i2 * 3 + 1] * scale, mesh->vertices[i2 * 3 + 2] * scale, 1);
			vec2 nPos = convert(pos, modelMatrix, viewMatrix, projectionMatrix, screenWidth, screenHeight);
			g2->drawImage(redDot, nPos.x(), nPos.y());
		}
	}
	
	g2->end();
}

void Avatar::drawJoints(const mat4& modelMatrix, const mat4& viewMatrix, const mat4& projectionMatrix, int screenWidth, int screenHeight, bool skeleton) {
	g2->begin(false);
	
	for(int i = 1; i < bones.size(); ++i) {
		BoneNode* bone = bones[i];
		vec4 pos = bone->combined * vec4(0, 0, 0, 1);
		vec2 bonePos = convert(pos, modelMatrix, viewMatrix, projectionMatrix, screenWidth, screenHeight);
		g2->drawImage(redDot, bonePos.x(), bonePos.y());
		
		BoneNode* parent = bone->parent;
		if (parent->nodeIndex > 2) {
			pos = parent->combined * vec4(0, 0, 0, 1);
			vec2 parentPos = convert(pos, modelMatrix, viewMatrix, projectionMatrix, screenWidth, screenHeight);
			g2->drawLine(bonePos.x(), bonePos.y(), parentPos.x(), parentPos.y(), 5);
			g2->drawRect(0, 0, 0, 0);
		}
	}
	
	// Draw desired position
	vec2 nPos = convert(desiredPosition, modelMatrix, viewMatrix, projectionMatrix, screenWidth, screenHeight);
	g2->drawImage(yellowDot, nPos.x(), nPos.y());
	
	g2->end();
}

vec3 Avatar::getBonePosition(int boneIndex) const {
	BoneNode* bone = getBoneWithIndex(boneIndex);
	vec4 pos = bone->combined * vec4(0, 0, 0, 1);
	pos *= 1.0/pos.w();
	return vec3(pos.x(), pos.y(), pos.z());
}

Quaternion Avatar::getBoneLocalRotation(int boneIndex) const {
	BoneNode* bone = getBoneWithIndex(boneIndex);
	return bone->quaternion;
}

Quaternion Avatar::getBoneGlobalRotation(int boneIndex) const {
	BoneNode* bone = getBoneWithIndex(boneIndex);
	Kore::Quaternion quat;
	Kore::RotationUtility::getOrientation(&bone->combined, &quat);
	return quat;
}

void Avatar::setDesiredPosition(int boneIndex, Kore::vec3 desiredPos) {
	setDesiredPositionAndOrientation(boneIndex, desiredPos, Kore::Quaternion(0, 0, 0, 0), false);
}

void Avatar::setDesiredPositionAndOrientation(int boneIndex, Kore::vec3 desiredPos, Kore::Quaternion desiredRot, bool posAndRot) {
	BoneNode* bone = getBoneWithIndex(boneIndex);
	desiredPosition = vec4(desiredPos.x(), desiredPos.y(), desiredPos.z(), 1.0);
	invKin->inverseKinematics(bone, desiredPosition, desiredRot, posAndRot);
}

void Avatar::setLocalRotation(int boneIndex, Kore::Quaternion desiredRotation) {
	BoneNode* bone = getBoneWithIndex(boneIndex);
	desiredRotation.normalize();
	bone->quaternion = desiredRotation;
	bone->interQuat = desiredRotation;
	Kore::mat4 rotMat = desiredRotation.matrix().Transpose();
	bone->local = bone->transform * rotMat;
}

float Avatar::getAverageIKiterationNum() const {
	return invKin->getAverageIter();
}

float Avatar::getHeight() const {
	return currentHeight;
}
