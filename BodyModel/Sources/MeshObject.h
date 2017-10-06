#pragma once

#include "OpenGEX/OpenGEX.h"

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Math/Quaternion.h>

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
	const char* name;
	const char* objectRef;
	const char* materialRef;
	unsigned int materialIndex;
	unsigned int geometryIndex;
};

struct CompareGeometry {
	bool const operator()(Geometry* geo1, Geometry* geo2) const {
		return (geo1->geometryIndex) < (geo2->geometryIndex);
	}
};

struct Light {
	Kore::vec4 position;
	const char* name;
	int type;
};

struct Material {
	const char* materialName;
	char* textureName;
	unsigned int materialIndex;
	
	int texScaleX;
	int texScaleY;
	
	Kore::vec3 diffuse;
	Kore::vec3 specular;
	float specular_power;
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
	
	Kore::Quaternion interQuat;
	Kore::Quaternion quaternion;	// local rotation
	
	bool initialized = false;
	
	std::vector<Kore::mat4> aniTransformations;
	
	// Constraints
	Kore::vec3 axes;
	std::vector<Kore::vec2> constrain;	// <min, max>
	
	BoneNode() : transform(Kore::mat4::Identity()), local(Kore::mat4::Identity()),
				 combined(Kore::mat4::Identity()), combinedInv(Kore::mat4::Identity()),
				 finalTransform(Kore::mat4::Identity()),
				 quaternion(Kore::Quaternion(0, 0, 0, 1)), interQuat(Kore::Quaternion(0, 0, 0, 1)),
				  axes(Kore::vec3(0, 0, 0)) {}
};

struct CompareBones {
	bool const operator()(BoneNode* bone1, BoneNode* bone2) const {
		return (bone1->nodeDepth) < (bone2->nodeDepth);
	}
};

class MeshObject {
public:
	MeshObject(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f);
	void render(Kore::Graphics4::TextureUnit tex);

	void setScale(float scaleFactor);
	Kore::mat4 M;
	
	long meshesCount;
	float scale;
	const Kore::Graphics4::VertexStructure& structure;
	Kore::Graphics4::VertexBuffer** vertexBuffers;
	Kore::Graphics4::IndexBuffer** indexBuffers;
	
	Kore::Graphics4::Texture** images;
	
	const char* textureDir;
	std::vector<Mesh*> meshes;
	std::vector<Geometry*> geometries;
	std::vector<Material*> materials;
	std::vector<BoneNode*> bones;
	std::vector<BoneNode*> children;
	std::vector<Light*> lights;

	BoneNode* getBoneWithIndex(int index) const;
	Material* findMaterialWithIndex(const int index);
	
private:	
	void LoadObj(const char* filename);
	
	void ConvertObjects(const Structure& structure);
	Mesh* ConvertGeometryObject(const OGEX::GeometryObjectStructure& structure);
	Mesh* ConvertMesh(const OGEX::MeshStructure& structure, const char* geometryName);
	
	Material* ConvertMaterial(const OGEX::MaterialStructure& structure);
	
	void ConvertNodes(const Structure& structure, BoneNode& parentNode);
	Geometry* ConvertGeometryNode(const OGEX::GeometryNodeStructure& structure);
	BoneNode* ConvertBoneNode(const OGEX::BoneNodeStructure& structure);
	
	Light* ConvertLightNode(const OGEX::LightNodeStructure& structure);
};
