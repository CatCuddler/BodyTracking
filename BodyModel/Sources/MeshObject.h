#pragma once

#include "OpenGEX/OpenGEX.h"

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics4/Texture.h>

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

class MeshObject {

public:
	MeshObject(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f);
	void render(Kore::Graphics4::TextureUnit tex);
    
    Kore::mat4 M;
	
	
private:
    long meshesCount;
    std::vector<Kore::Graphics4::VertexBuffer*> vertexBuffers;
	std::vector<Kore::Graphics4::IndexBuffer*> indexBuffers;
	
    const char* textureDir;
	std::vector<Kore::Graphics4::Texture*> images;
    std::vector<Mesh*> meshes;
	std::vector<Geometry*> geometries;
	std::vector<Material*> materials;
	
	void LoadObj(const char* filename);
	
	void ConvertObjectStructure(const Structure& structure);
	Mesh* ConvertGeometryObject(const OGEX::GeometryObjectStructure& structure);
	Mesh* ConvertMesh(const OGEX::MeshStructure& structure, const char* geometryName);
	
	void ConvertNodeStructure(const Structure& structure);
	Geometry* ConvertGeometryNode(const OGEX::GeometryNodeStructure& structure);
	
    Material* ConvertMaterial(const OGEX::MaterialStructure& structure);
};
