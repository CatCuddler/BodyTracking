#include "pch.h"
#include "Avatar.h"

using namespace Kore;
using namespace Kore::Graphics4;

Avatar::Avatar(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale) : MeshObject(meshFile, textureFile, structure, scale) {
	invKin = new InverseKinematics(bones);
	
	// Update bones
	for (int i = 0; i < bones.size(); ++i) invKin->initializeBone(bones[i]);
	
	// Get the highest position
	BoneNode* head = getBoneWithIndex(headBoneIndex);
	Kore::vec4 position = head->combined * Kore::vec4(0, 0, 0, 1);
	position *= 1.0/position.w();
	currentHeight = position.z();
}

void Avatar::animate(TextureUnit tex) {
	// Update bones
	for (int i = 0; i < bones.size(); ++i) invKin->initializeBone(bones[i]);
	
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

void Avatar::setDesiredPositionAndOrientation(int boneIndex, IKMode ikMode, Kore::vec3 desPosition, Kore::Quaternion desRotation) {
	BoneNode* bone = getBoneWithIndex(boneIndex);
	
	invKin->inverseKinematics(bone, ikMode, desPosition, desRotation);
}

void Avatar::setFixedPositionAndOrientation(int boneIndex, Kore::vec3 desPosition, Kore::Quaternion desRotation) {
	BoneNode* bone = getBoneWithIndex(boneIndex);
	
	bone->transform = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z());
	bone->rotation = desRotation;
	bone->rotation.normalize();
	bone->local = bone->transform * bone->rotation.matrix().Transpose();
}

BoneNode* Avatar::getBoneWithIndex(int boneIndex) const {
	BoneNode* bone = bones[boneIndex - 1];
	return bone;
}

void Avatar::resetPositionAndRotation() {
	for (int i = 0; i < bones.size(); ++i) {
		bones[i]->transform = bones[i]->bind;
		bones[i]->local = bones[i]->bind;
		bones[i]->combined = bones[i]->parent->combined * bones[i]->local;
		bones[i]->combinedInv = bones[i]->combined.Invert();
		bones[i]->finalTransform = bones[i]->combined * bones[i]->combinedInv;
		bones[i]->rotation = Kore::Quaternion(0, 0, 0, 1);
	}
}

float Avatar::getReached() const {
	return invKin->getReached();
}

float Avatar::getStucked() const {
	return invKin->getStucked();
}

float* Avatar::getIterations() const {
	return invKin->getIterations();
}

float* Avatar::getErrorPos() const {
	return invKin->getErrorPos();
}

float* Avatar::getErrorRot() const {
	return invKin->getErrorRot();
}

float* Avatar::getTime() const {
	return invKin->getTime();
}

float* Avatar::getTimeIteration() const {
	return invKin->getTimeIteration();
}

float Avatar::getHeight() const {
	return currentHeight;
}
