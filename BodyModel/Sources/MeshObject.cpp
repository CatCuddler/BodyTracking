#include "pch.h"
#include "MeshObject.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Log.h>

#include <cstring>

using namespace Kore;

MeshObject::MeshObject(const char* meshFile, const char* textureFile, const VertexStructure& structure, float scale) {
	
	LoadObj(meshFile);
	
	image = new Texture(textureFile, true);
	
	// Mesh Vertex Buffer
	vertexBuffer = new VertexBuffer(mesh->numVertices, structure, 0);
	float* vertices = vertexBuffer->lock();
	for (int i = 0; i < mesh->numVertices; ++i) {
		vertices[i * 8 + 0] = mesh->vertices[i * 8 + 0] * scale;
		vertices[i * 8 + 1] = mesh->vertices[i * 8 + 1] * scale;
		vertices[i * 8 + 2] = mesh->vertices[i * 8 + 2] * scale;
		vertices[i * 8 + 3] = mesh->vertices[i * 8 + 3];
		vertices[i * 8 + 4] = /*1.0f -*/ mesh->vertices[i * 8 + 4];
		vertices[i * 8 + 5] = mesh->vertices[i * 8 + 5];
		vertices[i * 8 + 6] = mesh->vertices[i * 8 + 6];
		vertices[i * 8 + 7] = mesh->vertices[i * 8 + 7];
        
        //log(Info, "%f %f %f %f %f %f %f %f", vertices[i * 8 + 0], vertices[i * 8 + 1], vertices[i * 8 + 2], vertices[i * 8 + 3], vertices[i * 8 + 4], vertices[i * 8 + 5], vertices[i * 8 + 6], vertices[i * 8 + 7]);
	}
	vertexBuffer->unlock();
	
	indexBuffer = new IndexBuffer(mesh->numFaces * 3);
	int* indices = indexBuffer->lock();
	for (int i = 0; i < mesh->numFaces * 3; ++i) {
		indices[i] = mesh->indices[i];
        
        //log(Info, "%i", indices[i]);
	}
	indexBuffer->unlock();
	
}

void MeshObject::render(TextureUnit tex) {
	Graphics::setTexture(tex, image);
	Graphics::setVertexBuffer(*vertexBuffer);
	Graphics::setIndexBuffer(*indexBuffer);
	Graphics::drawIndexedVertices();
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
		case OGEX::kStructureGeometryObject:
			mesh = ConvertGeometryObject(static_cast<const OGEX::GeometryObjectStructure&>(structure));
			break;
			
		case OGEX::kStructureLightObject:
			break;
			
		case OGEX::kStructureCameraObject:
			break;
			
		case OGEX::kStructureMaterial:
			break;
			
		default:
			log(Info, "Unknown object structure type");
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
			mesh->vertices[(i * 8) + 0] = data[i * 3 + 0];
			mesh->vertices[(i * 8) + 1] = data[i * 3 + 1];
			mesh->vertices[(i * 8) + 2] = data[i * 3 + 2];
			//log(Info, "Position %i \t x=%f \t y=%f \t z=%f", i, mesh->vertices[(i * 8) + 0], mesh->vertices[(i * 8) + 1], mesh->vertices[(i * 8) + 2]);
		}
	}
	
	void setUV(Mesh* mesh, int size, const float* data) {
		for (int i = 0; i < size; ++i) {
			mesh->vertices[(i * 8) + 3] = data[i * 2 + 0];
			mesh->vertices[(i * 8) + 4] = data[i * 2 + 1];
			//log(Info, "Texcoord %i \t u=%f \t v=%f", i, mesh->vertices[(i * 8) + 3], mesh->vertices[(i * 8) + 4]);
		}
	}
	
	void setNormal(Mesh* mesh, int size, const float* data) {
		for (int i = 0; i < size; ++i) {
			mesh->vertices[(i * 8) + 5] = data[i * 3 + 0];
			mesh->vertices[(i * 8) + 6] = data[i * 3 + 1];
			mesh->vertices[(i * 8) + 7] = data[i * 3 + 2];
			//log(Info, "Normal %i \t x=%f \t y=%f \t z=%f", i, mesh->vertices[(i * 8) + 5], mesh->vertices[(i * 8) + 6], mesh->vertices[(i * 8) + 7]);
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
				
				//int arraySize = vertexArrayStructure.GetArraySize();
				const float* data = vertexArrayStructure.GetData();
				int numVertices = vertexArrayStructure.GetVertexCount();
				
				if (mesh->vertices == nullptr) {
					mesh->vertices = new float[numVertices * 8];
				}
				
				if (arrayAttrib == "position") {
					setPosition(mesh, numVertices, data);
					mesh->numVertices = numVertices;
				} else if (arrayAttrib == "normal") {
					setNormal(mesh, numVertices, data);
					mesh->numNormals = numVertices;
				} else if (arrayAttrib == "texcoord") {
					setUV(mesh, numVertices, data);
					mesh->numUVs = numVertices;
				}
				
				//delete[] data;
				//data = nullptr;
			} break;
				
			case OGEX::kStructureIndexArray: {
				const OGEX::IndexArrayStructure& indexArrayStructure = *static_cast<const OGEX::IndexArrayStructure *>(subStructure);
				
				//int arraySize = indexArrayStructure.GetArraySize();
				int faces = indexArrayStructure.GetFaceCount();
				const unsigned_int32* data = indexArrayStructure.GetData();
				
				mesh->numFaces = faces;
				mesh->indices = new int[faces * 3];
				setIndex(mesh, faces, data);
				
			} break;
				
			default: break;
		}
		subStructure = subStructure->Next();
	}
	
	
	// TODO: Handle skin structure.
	
	return mesh;
}

/*
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int cmdShow)
{
	// Import the file "Code/Example.ogex".
 
	HANDLE handle = CreateFile("Code\\Example.ogex", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle != INVALID_HANDLE_VALUE)
	{
		OpenGexDataDescription	openGexDataDescription;
		DWORD					actual;
		
		DWORD size = GetFileSize(handle, nullptr);
		char *buffer = new char[size + 1];
		
		// Read the entire contents of the file and put a zero terminator at the end.
		
		ReadFile(handle, buffer, size, &actual, nullptr);
		buffer[size] = 0;
		
		// Once the file is in memory, the DataDescription::ProcessText() function
		// is called to create the structure tree and process the data.
		
		DataResult result = openGexDataDescription.ProcessText(buffer);
		if (result == kDataOkay)
		{
			const Structure *structure = openGexDataDescription.GetRootStructure()->GetFirstSubnode();
			while (structure)
			{
				// This loops over all top-level structures in the file.
				
				// Do something with the data...
				
				structure = structure->Next();
			}
		}
		
		delete[] buffer;
	}
 
	return (0);
}
*/
