#pragma once

#include "OpenGEX/OpenGEX.h"

#include <Kore/Graphics/Graphics.h>
#include <Kore/Graphics/Texture.h>

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
};

struct Geometry {
	Kore::mat4 transform;
	char* name;
	const char* objectRef;
	const char* materialRef;
	
};

struct Material {
	char* materialName;
	char* textureName;
};

class MeshObject {

public:
	MeshObject(const char* meshFile, const char* textureFile, const Kore::VertexStructure& structure, float scale = 1.0f);
	void render(Kore::TextureUnit tex);
    
    Kore::mat4 M;
	
	
private:
    long meshesCount;
    std::vector<Kore::VertexBuffer*> vertexBuffers;
	std::vector<Kore::IndexBuffer*> indexBuffers;
	
    const char* textureDir;
    std::vector<Kore::Texture*> images;
    std::vector<Mesh*> meshes;
	std::vector<Geometry*> geometries;
	
	void LoadObj(const char* filename);
	
	void ConvertObjectStructure(const Structure& structure);
	Mesh* ConvertGeometryObject(const OGEX::GeometryObjectStructure& structure);
	Mesh* ConvertMesh(const OGEX::MeshStructure& structure);
	
	void ConvertNodeStructure(const Structure& structure);
	Geometry* ConvertGeometryNode(const OGEX::GeometryNodeStructure& structure);
	
    Material* ConvertMaterial(const OGEX::MaterialStructure& structure) ;
};
