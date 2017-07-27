#pragma once

#include "OpenGEX/OpenGEX.h"

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics2/Graphics.h>
#include <Kore/Graphics4/Texture.h>
#include <Kore/Math/Quaternion.h>

#include "InverseKinematics.h"

#include <vector>

struct Mesh {
	int numFaces;
	int numVertices;
	int numUVs;
	int numNormals;
	
	float* vertices;
	int* indices;
	float* normals;
	float* texcoord;
	
	// Skin
	unsigned_int16* boneCountArray;
	int boneCount;
	unsigned_int16* boneIndices;
	int boneIndexCount;
	float* boneWeight;
	int weightCount;
	
	unsigned int meshIndex;
};

struct CompareMesh {
	bool const operator()(Mesh* mesh1, Mesh* mesh2) const {
		return (mesh1->meshIndex) < (mesh2->meshIndex);
	}
};

struct Geometry {
	Kore::mat4 transform;
	char* name;
	const char* objectRef;
	const char* materialRef;
	unsigned int geometryIndex;
};

struct CompareGeometry {
	bool const operator()(Geometry* geo1, Geometry* geo2) const {
		return (geo1->geometryIndex) < (geo2->geometryIndex);
	}
};

struct Material {
	char* materialName;
	char* textureName;
	unsigned int materialIndex;
};

struct CompareMaterials {
	bool const operator()(Material* material1, Material* material2) const {
		return (material1->materialIndex) < (material2->materialIndex);
	}
};

struct BoneNode {
	char* boneName;
	int nodeIndex;
	int nodeDepth;
	BoneNode* parent;
	
	Kore::mat4 transform;		// bind matrix
	Kore::mat4 local;
	Kore::mat4 combined, combinedInv;
	Kore::mat4 finalTransform;
	
	Kore::Quaternion quaternion;	// local rotation
	
	bool initialized = false;
	
	std::vector<Kore::mat4> aniTransformations;
	
	// Constraints
	Kore::vec3 axes;
	std::vector<Kore::vec2> constrain;	// <min, max>
	
	BoneNode() : transform(Kore::mat4::Identity()), local(Kore::mat4::Identity()),
				 combined(Kore::mat4::Identity()), combinedInv(Kore::mat4::Identity()),
				 finalTransform(Kore::mat4::Identity()),
				 quaternion(Kore::Quaternion(0, 0, 0, 1)) {}
};

struct CompareBones {
	bool const operator()(BoneNode* bone1, BoneNode* bone2) const {
		return (bone1->nodeDepth) < (bone2->nodeDepth);
	}
};

class InverseKinematics;

class MeshObject {
public:
	MeshObject(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f);
	void render(Kore::Graphics4::TextureUnit tex);
	
	void setAnimation(int frame);
	void setDesiredPosition(int boneIndex, Kore::vec3 desiredPos);
	void setDesiredPositionAndOrientation(int boneIndex, Kore::vec3 desiredPos, Kore::Quaternion desiredRot);
	void animate(Kore::Graphics4::TextureUnit tex, float deltaTime);
	
	vec3 getBonePosition(int boneIndex);
	Quaternion getBoneLocalRotation(int boneIndex);
	Quaternion getBoneGlobalRotation(int boneIndex);
	
	void setLocalRotation(int boneIndex, Kore::Quaternion desiredRotation);

	void drawJoints(const Kore::mat4& modelMatrix, const Kore::mat4& viewMatrix, const Kore::mat4& projectionMatrix, int screenWidth, int screenHeight, bool skeleton);
	void drawVertices(const Kore::mat4& modelMatrix, const Kore::mat4& viewMatrix, const Kore::mat4& projectionMatrix, int screenWidth, int screenHeight);
	
	float getHeight();
	void setScale(float scaleFactor);
	Kore::mat4 M;
	
private:
	Kore::vec4 desiredPosition;
	
	bool logData = false;
	
	long meshesCount;
	float scale;
	float currentHeight;
	const Kore::Graphics4::VertexStructure& structure;
	std::vector<Kore::Graphics4::VertexBuffer*> vertexBuffers;
	std::vector<Kore::Graphics4::IndexBuffer*> indexBuffers;
	
	InverseKinematics* invKin;
	const int maxIteration = 100;
	
	Kore::Graphics2::Graphics2* g2;
	Kore::Graphics4::Texture* redDot;
	Kore::Graphics4::Texture* yellowDot;
	
	const char* textureDir;
	std::vector<Kore::Graphics4::Texture*> images;
	std::vector<Mesh*> meshes;
	std::vector<Geometry*> geometries;
	std::vector<Material*> materials;
	std::vector<BoneNode*> bones;
	std::vector<BoneNode*> children;
	
	void LoadObj(const char* filename);
	
	BoneNode* getBoneWithIndex(int index);
	
	void ConvertObjects(const Structure& structure);
	Mesh* ConvertGeometryObject(const OGEX::GeometryObjectStructure& structure);
	Mesh* ConvertMesh(const OGEX::MeshStructure& structure, const char* geometryName);
	
	Material* ConvertMaterial(const OGEX::MaterialStructure& structure);
	
	void ConvertNodes(const Structure& structure, BoneNode& parentNode);
	Geometry* ConvertGeometryNode(const OGEX::GeometryNodeStructure& structure);
	BoneNode* ConvertBoneNode(const OGEX::BoneNodeStructure& structure);
};
