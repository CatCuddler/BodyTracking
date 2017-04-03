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
	
	void LoadObj(const char* filename);
	void ConvertObjectStructure(const Structure& structure);
	Mesh* ConvertGeometryObject(const OGEX::GeometryObjectStructure& structure);
	Mesh* ConvertMesh(const OGEX::MeshStructure& structure);
	
    const char* ConvertMaterial(const OGEX::MaterialStructure& structure) ;
};
