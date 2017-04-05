#include "pch.h"
#include "MeshObject.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Log.h>

#include <sstream>
#include <algorithm>
#include <functional>

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
    void setPosition(Mesh* mesh, int size, const float* data) {
        for (int i = 0; i < size; ++i) {
            mesh->vertices[(i * 3) + 0] = data[i * 3 + 0];
            mesh->vertices[(i * 3) + 1] = data[i * 3 + 1];
            mesh->vertices[(i * 3) + 2] = data[i * 3 + 2];
            //if (i < 5 || i > size-5)
			//	log(Info, "Position %i \t x=%f \t y=%f \t z=%f", i, mesh->vertices[(i * 3) + 0], mesh->vertices[(i * 3) + 1], mesh->vertices[(i * 3) + 2]);
        }
    }
    
    void setTexcoord(Mesh* mesh, int size, const float* data) {
        for (int i = 0; i < size; ++i) {
            mesh->texcoord[(i * 2) + 0] = data[i * 2 + 0];
            mesh->texcoord[(i * 2) + 1] = data[i * 2 + 1];
			//if (i < 5 || i > size-5)
			//	log(Info, "Texcoord %i \t u=%f \t v=%f", i, mesh->texcoord[(i * 2) + 0], mesh->texcoord[(i * 2) + 1]);
        }
    }
    
    void setNormal(Mesh* mesh, int size, const float* data) {
        for (int i = 0; i < size; ++i) {
            mesh->normals[(i * 3) + 0] = data[i * 3 + 0];
            mesh->normals[(i * 3) + 1] = data[i * 3 + 1];
            mesh->normals[(i * 3) + 2] = data[i * 3 + 2];
            //if (i < 5 || i > size-5)
			//	log(Info, "Normal %i \t x=%f \t y=%f \t z=%f", i, mesh->normals[(i * 3) + 0], mesh->normals[(i * 3) + 1], mesh->normals[(i * 3) + 2]);
        }
    }
    
    void setIndex(Mesh* mesh, int size, const unsigned_int32* data) {
        for (int i = 0; i < size; ++i) {
            mesh->indices[(i * 3) + 0] = data[i * 3 + 0];
            mesh->indices[(i * 3) + 1] = data[i * 3 + 1];
            mesh->indices[(i * 3) + 2] = data[i * 3 + 2];
			//if (i < 5 || i > size-5)
			//	log(Info, "Index %i \t x=%i \t y=%i \t z=%i", i, mesh->indices[(i * 3) + 0], mesh->indices[(i * 3) + 1], mesh->indices[(i * 3) + 2]);
        }
    }
	
	void setWeight(Mesh* mesh, int size, const float* data) {
		for (int i = 0; i < size; ++i) {
			mesh->weight[i] = data[i];
			//if (i < 5 || i > size-5)
			//	log(Info, "Index %i \t w=%f", i, mesh->weight[i]);
		}
	}
	
	int getIndexFromString(const char* name, int ignore) {
		const char* num = name + ignore;
		std::stringstream strValue;
		strValue << num;
		unsigned int intValue;
		strValue >> intValue;
		return intValue;
	}
	
	void copyString(const char* from, char* to, int length) {
		for (int i = 0; i < length; ++i) {
			to[i] = from[i];
		}
	}
	
	mat4 getMatrix4x4(const float* matrix) {
		mat4 mat = mat4::Identity();
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				mat.Set(i, j, matrix[i + 4 * j]);
			}
		}
		return mat;
	}
}

MeshObject::MeshObject(const char* meshFile, const char* textureFile, const VertexStructure& structure, float scale) : textureDir(textureFile) , M(mat4::Identity()) {
	
	LoadObj(meshFile);
    
    meshesCount = meshes.size();
    log(Info, "Meshes length %i", meshesCount);
	
	std::sort(meshes.begin(), meshes.end(), CompareMesh());
	std::sort(geometries.begin(), geometries.end(), CompareGeometry());
	std::sort(materials.begin(), materials.end(), CompareMaterials());
    
    
    for(int j = 0; j < meshesCount; ++j) {
        Mesh* mesh = meshes.at(j);
        VertexBuffer * vertexBuffer = new VertexBuffer(mesh->numVertices, structure, 0);
        IndexBuffer* indexBuffer = new IndexBuffer(mesh->numFaces*3);
        
        // Mesh Vertex Buffer
        float* vertices = vertexBuffer->lock();
        for (int i = 0; i < mesh->numVertices; ++i) {
            // position
            vertices[i * 8 + 0] = mesh->vertices[i * 3 + 0] * scale;
            vertices[i * 8 + 1] = mesh->vertices[i * 3 + 1] * scale;
            vertices[i * 8 + 2] = mesh->vertices[i * 3 + 2] * scale;
            // texCoord
            vertices[i * 8 + 3] = mesh->texcoord[i * 2 + 0];
            vertices[i * 8 + 4] = 1.0f - mesh->texcoord[i * 2 + 1];
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
		
		Material* material = materials.at(j);
		char temp[200];
		strcpy (temp, textureDir);
		std::strcat(temp, material->textureName);
		log(Info, "Load Texture %s", temp);
		Texture* image = new Texture(temp, true);
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
        
        Graphics4::setTexture(tex, image);
        Graphics4::setVertexBuffer(*vertexBuffer);
        Graphics4::setIndexBuffer(*indexBuffer);
        Graphics4::drawIndexedVertices();
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
		//const Structure* structure = openGexDataDescription.GetRootStructure()->GetFirstSubnode();
		//while (structure) {
			// This loops over all top-level structures in the file.
			
			// Do something with the data...
			ConvertObjects(*openGexDataDescription.GetRootStructure());
			ConvertNodes(*openGexDataDescription.GetRootStructure());
			
			//structure = structure->Next();
		//}
	} else {
		log(Info, "Failed to load OpenGEX file");
	}
	
	delete[] buffer;
}

void MeshObject::ConvertObjects(const Structure& rootStructure) {
	const Structure* structure = rootStructure.GetFirstSubnode();
	while (structure) {
		switch (structure->GetStructureType()) {
			case OGEX::kStructureGeometryObject: {
				Mesh* mesh = ConvertGeometryObject(static_cast<const OGEX::GeometryObjectStructure&>(*structure));
				meshes.push_back(mesh);
				break;
			}
				
			case OGEX::kStructureLightObject:
				break;
				
			case OGEX::kStructureCameraObject:
				break;
				
			case OGEX::kStructureMaterial: {
				Material* material = ConvertMaterial(static_cast<const OGEX::MaterialStructure&>(*structure));
				materials.push_back(material);
				break;
			}
				
			default:
				//log(Info, "Unknown object structure type");
				break;
		}
		
		structure = structure->Next();
	}
}

void MeshObject::ConvertNodes(const Structure& rootStructure) {
	const Structure* structure = rootStructure.GetFirstSubnode();
	while (structure) {
		
		ConvertNodeStructure(*static_cast<const OGEX::NodeStructure*>(structure));
		ConvertNodes(*structure);
		
		structure = structure->Next();
	}
}

void MeshObject::ConvertNodeStructure(const OGEX::NodeStructure& nodeStructure) {
	switch (nodeStructure.GetStructureType()) {
		case OGEX::kStructureNode:
			//return ConvertNode(static_cast<const OGEX::NodeStructure&>(structure));
			
		case OGEX::kStructureBoneNode: {
			BoneNode* bone = ConvertBoneNode(static_cast<const OGEX::BoneNodeStructure&>(nodeStructure));
			bones.push_back(bone);
			break;
		}
			
		case OGEX::kStructureGeometryNode: {
			Geometry* geometry = ConvertGeometryNode(static_cast<const OGEX::GeometryNodeStructure&>(nodeStructure));
			geometries.push_back(geometry);
			break;
		}
			
		case OGEX::kStructureLightNode:
			//return ConvertLightNode(static_cast<const OGEX::LightNodeStructure&>(structure));
			
		case OGEX::kStructureCameraNode:
			//return ConvertCameraNode(static_cast<const CameraNodeStructure&>(nodeStructure));
			
		default:
			//log(Info, "Unknown node structure type");
			break;
	}
}


Mesh* MeshObject::ConvertGeometryObject(const OGEX::GeometryObjectStructure& structure) {
	const Map<OGEX::MeshStructure>* meshMap = structure.GetMeshMap();
	
	const char* geometryName = structure.GetStructureName();
	
	if (meshMap == nullptr) {
		log(Info, "Invalid geometry mesh structure");
		return nullptr;
	}
	// TODO: Handle morph structures.
	const OGEX::MeshStructure* meshStructure = meshMap->First();
	while (meshStructure) {
		if (meshStructure->GetKey() == 0) break;
		meshStructure = meshStructure->Next();
	}
	
	return ConvertMesh(*meshStructure, geometryName);
}

Mesh* MeshObject::ConvertMesh(const OGEX::MeshStructure& structure, const char* geometryName) {
	Mesh* mesh = new Mesh();
	mesh->meshIndex = getIndexFromString(geometryName, 8);
	
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
				
			case OGEX::kStructureSkin : {
				const OGEX::SkinStructure& skinStructure = *static_cast<const OGEX::SkinStructure *>(subStructure);

				// TODO: Handle skin structure.
				// setAnimations und render
				
				// Get weight array
				const Structure *subStructure = skinStructure.GetFirstSubstructure(OGEX::kStructureBoneWeightArray);
				const OGEX::BoneWeightArrayStructure& weightStructure = *static_cast<const OGEX::BoneWeightArrayStructure *>(subStructure);
				const float* weights = weightStructure.GetBoneWeightArray();
				
				int weightCount = weightStructure.GetBoneWeightCount();
				mesh->weightCount = weightCount;
				//log(Info, "Weight count %i", mesh->weightCount);
				
				mesh->weight = new float[weightCount];
				setWeight(mesh, weightCount, weights);
			}
				
			default: break;
		}
		subStructure = subStructure->Next();
	}
	
	
	// TODO: Handle skin structure.
	
	return mesh;
}

Geometry* MeshObject::ConvertGeometryNode(const OGEX::GeometryNodeStructure& structure) {
	Geometry* geometry = new Geometry();
	
	const char* name = structure.GetNodeName();
	int length = (int)strlen(name) + 1;
	geometry->name = new char[length]();
	copyString(name, geometry->name, length);
	//log(Info, "Geometry name %s", name);
	
	const char* nodeName = structure.GetStructureName();
	geometry->geometryIndex = getIndexFromString(nodeName, 4);
	
	const Structure *subStructure = structure.GetFirstSubnode();
	while (subStructure) {
		switch (subStructure->GetStructureType()) {
			case OGEX::kStructureTransform: {
				const OGEX::TransformStructure& transformStructure = *static_cast<const OGEX::TransformStructure *>(subStructure);
				const float* transform = transformStructure.GetTransform();
				geometry->transform = getMatrix4x4(transform);
				
				break;
			}
				
			default:
				break;
		}
		subStructure = subStructure->Next();
	}
	
	return geometry;
}

Material* MeshObject::ConvertMaterial(const OGEX::MaterialStructure& materialStructure) {
	Material* material = new Material();
	
	const char* name = materialStructure.GetStructureName();
	int length = (int)strlen(name) + 1;
	material->materialName = new char[length]();
	copyString(name, material->materialName, length);
	//log(Info, "Material name %s", name);
	
	material->materialIndex = getIndexFromString(name, 8);
	
	const Structure* subStructure = materialStructure.GetFirstSubnode();
	while (subStructure) {
		
		switch (subStructure->GetStructureType()) {
				
			case OGEX::kStructureColor:
				break;
				
			case OGEX::kStructureParam:
				break;
				
			case OGEX::kStructureTexture: {
				const OGEX::TextureStructure& textureStructure = *static_cast<const OGEX::TextureStructure *>(subStructure);
				
				const String& attrib = textureStructure.GetAttribString();
				if (attrib == "diffuse") {
					const char* textureName = static_cast<const char*>(textureStructure.GetTextureName());
					textureName += 2;
					int length = (int)strlen(textureName) + 1;
					material->textureName = new char[length]();
					copyString(textureName, material->textureName, length);
					//log(Info, "Texture name %s", material->textureName);
				}
				
				break;
			}
				
			default:
				break;
		}
		
		subStructure = subStructure->Next();
	}
	
	return material;
}

BoneNode* MeshObject::ConvertBoneNode(const OGEX::BoneNodeStructure& structure) {
	BoneNode* bone = new BoneNode();
	
	const char* name = structure.GetNodeName();
	int length = (int)strlen(name) + 1;
	bone->boneName = new char[length]();
	copyString(structure.GetNodeName(), bone->boneName, length);
	
	const char* nodeName = structure.GetStructureName();
	bone->nodeIndex = getIndexFromString(nodeName, 4);
	
	//log(Info, "Bone %s with index %i", bone->boneName, bone->nodeIndex);
	
	const Structure *subStructure = structure.GetFirstSubstructure(OGEX::kStructureTransform);
	const OGEX::TransformStructure& transformStructure = *static_cast<const OGEX::TransformStructure *>(subStructure);
	const float* transform = transformStructure.GetTransform();
	bone->local = getMatrix4x4(transform);
	
	return bone;
}
