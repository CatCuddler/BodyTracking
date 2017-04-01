#include "pch.h"
#include "MeshObject.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Log.h>

#include <cstring>

using namespace Kore;

MeshObject::MeshObject(const char* meshFile, const char* textureFile, const VertexStructure& structure, float scale) {
	
	LoadObj(meshFile);
    
    meshesCount = meshes.size();
    log(Info, "Meshes length %i", meshesCount);
    
    
    for(int j = 0; j < meshes.size(); ++j) {
        Mesh* mesh = meshes.at(j);
        VertexBuffer * vertexBuffer = new VertexBuffer(mesh->numVertices, structure, 0);
        IndexBuffer* indexBuffer = new IndexBuffer(mesh->numFaces*3);
        
        // Mesh Vertex Buffer
        float* vertices = vertexBuffer->lock();
        for (int i = j; i < mesh->numVertices; ++i) {
            // position
            vertices[i * 8 + 0] = mesh->vertices[i * 3 + 0] * scale;
            vertices[i * 8 + 1] = mesh->vertices[i * 3 + 1] * scale;
            vertices[i * 8 + 2] = mesh->vertices[i * 3 + 2] * scale;
            // texCoord
            vertices[i * 8 + 3] = mesh->texcoord[i * 2 + 0];
            vertices[i * 8 + 4] = /*1.0f -*/ mesh->texcoord[i * 2 + 1];
            // normal
            vertices[i * 8 + 5] = mesh->normals[i * 3 + 0];
            vertices[i * 8 + 6] = mesh->normals[i * 3 + 1];
            vertices[i * 8 + 7] = mesh->normals[i * 3 + 2];
            
            //log(Info, "%f %f %f %f %f %f %f %f", vertices[i * 8 + 0], vertices[i * 8 + 1], vertices[i * 8 + 2], vertices[i * 8 + 3], vertices[i * 8 + 4], vertices[i * 8 + 5], vertices[i * 8 + 6], vertices[i * 8 + 7]);
        }
        vertexBuffer->unlock();
        
        int* indices = indexBuffer->lock();
        for (int i = j; i < mesh->numFaces * 3; ++i) {
            indices[i] = mesh->indices[i];
            
            //log(Info, "%i", indices[i]);
        }
        indexBuffer->unlock();
        
        Texture* image = new Texture(textureFile, true);
        images.push_back(image);
        
        vertexBuffers.push_back(vertexBuffer);
        indexBuffers.push_back(indexBuffer);
    }
	
}

void MeshObject::render(TextureUnit tex) {
    for (int i = 0; i < meshesCount; ++i) {
        VertexBuffer* vertexBuffer = vertexBuffers.at(i);
        IndexBuffer* indexBuffer = indexBuffers.at(i);
        Texture* image = images.at(i);
        
        Graphics::setTexture(tex, image);
        Graphics::setVertexBuffer(*vertexBuffer);
        Graphics::setIndexBuffer(*indexBuffer);
        Graphics::drawIndexedVertices();
    }
}

void MeshObject::LoadObj(const char* filename) {
	FileReader fileReader(filename, FileReader::Asset);
	void* data = fileReader.readAll();
	int size = fileReader.size() + 1;
	char* buffer = new char[size + 1];
	for (int i = 0; i < size; ++i) buffer[i] = reinterpret_cast<char*>(data)[i];
	buffer[size] = 0;
	
	OGEX::OpenGexDataDescription openGexDataDescription;
	DataResult result = openGexDataDescription.ProcessText(buffer);
	if (result == kDataOkay) {
		const Structure* structure = openGexDataDescription.GetRootStructure()->GetFirstSubnode();
		while (structure) {
			// This loops over all top-level structures in the file.
			
			// Do something with the data...
			ConvertObjectStructure(*structure);
			
			structure = structure->Next();
		}
	} else {
		log(Info, "Failed to load OpenGEX file");
	}
	
	delete[] buffer;
}

void MeshObject::ConvertObjectStructure(const Structure& structure) {
	switch (structure.GetStructureType()) {
        case OGEX::kStructureGeometryObject: {
			Mesh* mesh = ConvertGeometryObject(static_cast<const OGEX::GeometryObjectStructure&>(structure));
            meshes.push_back(mesh);
			break;
        }
			
		case OGEX::kStructureLightObject:
			break;
			
		case OGEX::kStructureCameraObject:
			break;
			
		case OGEX::kStructureMaterial:
			break;
			
		default:
			//log(Info, "Unknown object structure type");
			break;
	}
}

Mesh* MeshObject::ConvertGeometryObject(const OGEX::GeometryObjectStructure& structure) {
	const Map<OGEX::MeshStructure>* meshMap = structure.GetMeshMap();
	
	if (meshMap == nullptr) {
		log(Info, "Invalid geometry mesh structure");
		return nullptr;
	}
	// TODO: Handle morph structures.
	const OGEX::MeshStructure* meshStructure = meshMap->First();
	while (meshStructure) {
		// The geometry structure may contain a mesh for each LOD, but we only care
		// about the first/highest LOD.
		if (meshStructure->GetKey() == 0) break;
		meshStructure = meshStructure->Next();
	}
	
	return ConvertMesh(*meshStructure);
}

namespace {
	void setPosition(Mesh* mesh, int size, const float* data) {
		for (int i = 0; i < size; ++i) {
			mesh->vertices[(i * 3) + 0] = data[i * 3 + 0];
			mesh->vertices[(i * 3) + 1] = data[i * 3 + 1];
			mesh->vertices[(i * 3) + 2] = data[i * 3 + 2];
			//log(Info, "Position %i \t x=%f \t y=%f \t z=%f", i, mesh->vertices[(i * 8) + 0], mesh->vertices[(i * 8) + 1], mesh->vertices[(i * 8) + 2]);
		}
	}
	
	void setTexcoord(Mesh* mesh, int size, const float* data) {
		for (int i = 0; i < size; ++i) {
			mesh->texcoord[(i * 2) + 0] = data[i * 2 + 0];
			mesh->texcoord[(i * 2) + 1] = data[i * 2 + 1];
			//log(Info, "Texcoord %i \t u=%f \t v=%f", i, mesh->uvs[(i * 2) + 0], mesh->uvs[(i * 2) + 1]);
		}
	}
	
	void setNormal(Mesh* mesh, int size, const float* data) {
		for (int i = 0; i < size; ++i) {
            mesh->normals[(i * 3) + 0] = data[i * 3 + 0];
            mesh->normals[(i * 3) + 1] = data[i * 3 + 1];
            mesh->normals[(i * 3) + 2] = data[i * 3 + 2];
            //log(Info, "Normal %i \t x=%f \t y=%f \t z=%f", i, mesh->normals[(i * 3) + 0], mesh->normals[(i * 3) + 1], mesh->normals[(i * 3) + 2]);
		}
	}
	
	void setIndex(Mesh* mesh, int size, const unsigned_int32* data) {
		for (int i = 0; i < size; ++i) {
			mesh->indices[(i * 3) + 0] = data[i * 3 + 0];
			mesh->indices[(i * 3) + 1] = data[i * 3 + 1];
			mesh->indices[(i * 3) + 2] = data[i * 3 + 2];
			//log(Info, "Index %i \t x=%i \t y=%i \t z=%i", i, mesh->indices[(i * 3) + 0], mesh->indices[(i * 3) + 1], mesh->indices[(i * 3) + 2]);
		}
	}
}

Mesh* MeshObject::ConvertMesh(const OGEX::MeshStructure& structure) {
	Mesh* mesh = new Mesh();
	mesh->vertices = nullptr;
	
	const Structure *subStructure = structure.GetFirstSubnode();
	while (subStructure) {
		switch (subStructure->GetStructureType()) {
			case OGEX::kStructureVertexArray: {
				const OGEX::VertexArrayStructure& vertexArrayStructure = *static_cast<const OGEX::VertexArrayStructure *>(subStructure);
				const String& arrayAttrib = vertexArrayStructure.GetArrayAttrib();
				
				int arraySize = vertexArrayStructure.GetArraySize();
				const float* data = vertexArrayStructure.GetData();
				int numVertices = vertexArrayStructure.GetVertexCount();
				
				if (arrayAttrib == "position") {
                    mesh->vertices = new float[numVertices * arraySize];
					setPosition(mesh, numVertices, data);
					mesh->numVertices = numVertices;
				} else if (arrayAttrib == "normal") {
                    mesh->normals = new float[numVertices * arraySize];
					setNormal(mesh, numVertices, data);
					mesh->numNormals = numVertices;
				} else if (arrayAttrib == "texcoord") {
                    mesh->texcoord = new float[numVertices * arraySize];
					setTexcoord(mesh, numVertices, data);
					mesh->numUVs = numVertices;
				}
				
			} break;
				
			case OGEX::kStructureIndexArray: {
				const OGEX::IndexArrayStructure& indexArrayStructure = *static_cast<const OGEX::IndexArrayStructure *>(subStructure);
				
				int arraySize = indexArrayStructure.GetArraySize();
				int faces = indexArrayStructure.GetFaceCount();
				const unsigned_int32* data = indexArrayStructure.GetData();
				
				mesh->numFaces = faces;
				mesh->indices = new int[faces * arraySize];
				setIndex(mesh, faces, data);
				
			} break;
				
			default: break;
		}
		subStructure = subStructure->Next();
	}
	
	
	// TODO: Handle skin structure.
	
	return mesh;
}
