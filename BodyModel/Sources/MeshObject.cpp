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
	
	template <typename T>
	void cloneArray(const T* source, int size, T** dest) {
		*dest = new T[size];
		std::copy(source, source + size, *dest);
	}
	
	/*void setBoneCountArray(Mesh* mesh, int size, const unsigned_int16* data) {
		for (int i = 0; i < size; ++i) {
	 mesh->boneCountArray[i] = data[i];
	 //if (i < 5 || i > size-5)
	 //	log(Info, "Bone Count %i \t w=%i", i, mesh->boneCountArray[i]);
		}
	 }
	 
	 void setWeight(Mesh* mesh, int size, const float* data) {
		for (int i = 0; i < size; ++i) {
	 mesh->boneWeight[i] = data[i];
	 //if (i < 5 || i > size-5)
	 //	log(Info, "Weight %i \t w=%f", i, mesh->weight[i]);
		}
	 }
	 
	 void setBoneIndices(Mesh* mesh, int size, const unsigned_int16* data) {
		for (int i = 0; i < size; ++i) {
	 mesh->boneIndices[i] = data[i];
	 //if (i < 5 || i > size-5)
	 //	log(Info, "Bone Index %i \t i=%i", i, mesh->boneIndices[i]);
		}
	 }*/
	
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
				mat.Set(j, i, matrix[i + 4 * j]);
			}
		}
		return mat;
	}
	
	void updateBone(BoneNode* bone) {
		//if (bone->computed) return;
		
		//log(Info, "%i Update bone %s", bone->nodeIndex, bone->boneName);
		//bone->computed = true;
		bone->combined = bone->parent->combined * bone->local;
		
		if (!bone->initialized) {
			//log(Info, "%i Init bone %s", bone->nodeIndex, bone->boneName);
			bone->initialized = true;
			bone->combinedInv = bone->combined.Invert();
			bone->localStart = bone->local;
			bone->localStartInv = bone->localStart.Invert();
		}
		
		bone->finalTransform = bone->combined * bone->combinedInv;
		//for (int i = 0; i < bone->children.size(); ++i) updateBone(bone->children[i]);
	}
	
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

MeshObject::MeshObject(const char* meshFile, const char* textureFile, const VertexStructure& structure, float scale) : textureDir(textureFile), structure(structure), scale(scale), M(mat4::Identity()) {
	
	LoadObj(meshFile);
	
	meshesCount = meshes.size();
	log(Info, "Meshes length %i", meshesCount);
	
	std::sort(meshes.begin(), meshes.end(), CompareMesh());
	std::sort(geometries.begin(), geometries.end(), CompareGeometry());
	std::sort(materials.begin(), materials.end(), CompareMaterials());
	
	// Check bone children
	for (int i = 0; i < bones.size(); ++i) {
		BoneNode* bone = bones.at(i);
		std::sort(bone->children.begin(), bone->children.end(), CompareBones());
		/*log(Info, "Children for %s", bone->boneName);
		for (int i2 = 0; i2 < bone->children.size(); ++i2) {
			BoneNode* child = bone->children.at(i2);
			log(Info, "\t Child name %s", child->boneName);
		}*/
	}
	
	//if (bones.size() > 0)
	//	updateBone(bones.at(1), mat4::Identity(), true);
	
	for(int j = 0; j < meshesCount; ++j) {
		Mesh* mesh = meshes.at(j);
		VertexBuffer* vertexBuffer = new VertexBuffer(mesh->numVertices, structure, 0);
		IndexBuffer* indexBuffer = new IndexBuffer(mesh->numFaces * 3);
		
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
		for (int i = 0; i < mesh->numFaces * 3; ++i) {
			indices[i] = mesh->indices[i];
			
			//log(Info, "%i", indices[i]);
		}
		indexBuffer->unlock();
		
		vertexBuffers.push_back(vertexBuffer);
		indexBuffers.push_back(indexBuffer);
		
		Material* material = materials.at(j);
		char temp[200];
		strcpy (temp, textureDir);
		std::strcat(temp, material->textureName);
		log(Info, "Load Texture %s", temp);
		Texture* image = new Texture(temp, true);
		images.push_back(image);
	}
	
	g2 = new Graphics2::Graphics2(1024, 768);
	redDot = new Texture("redDot.png");
	
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

void MeshObject::drawJoints(const mat4& modelMatrix, const mat4& viewMatrix, const mat4& projectionMatrix, int screenWidth, int screenHeight) {
	g2->begin(false);
	
	for(int i = 1; i < bones.size(); ++i) {
		BoneNode* bone = bones.at(i);
		
		if (strcmp(bone->boneName, "upperarm_r") != 0) continue;
		
		//vec4 pos = vec4(bone->combined.get(3, 0), bone->combined.get(3, 1), bone->combined.get(3, 2), bone->combined.get(3, 3));
		vec4 pos = vec4(bone->bonePos.x(), bone->bonePos.y(), bone->bonePos.z(), 1);
		vec2 nPos = convert(pos, modelMatrix, viewMatrix, projectionMatrix, screenWidth, screenHeight);
		g2->drawImage(redDot, nPos.x(), nPos.y());
	}
	
	g2->end();
}

void MeshObject::drawVertices(const mat4& modelMatrix, const Kore::mat4& viewMatrix, const Kore::mat4& projectionMatrix, int screenWidth, int screenHeight) {
	g2->begin(false);
	
	for (int i = 0; i < meshesCount; ++i) {
		Mesh* mesh = meshes.at(i);
		for (int i2 = 0; i2 < mesh->numVertices; ++i2) {
			vec4 pos = vec4(mesh->vertices[i2 * 3 + 0] * scale, mesh->vertices[i2 * 3 + 1] * scale, mesh->vertices[i2 * 3 + 2] * scale, 1);
			vec2 nPos = convert(pos, modelMatrix, viewMatrix, projectionMatrix, screenWidth, screenHeight);
			g2->drawImage(redDot, nPos.x(), nPos.y());
		
		}
	}
	
	g2->end();
}

void MeshObject::setAnimation(int frame) {
	
	for(int i = 0; i < bones.size(); ++i) {
		BoneNode* bone = bones.at(i);
		bone->computed = false;
	}
	
	for(int i = 0; i < bones.size(); ++i) {
		BoneNode* bone = bones.at(i);
		
		if (strcmp(bone->boneName, "Root") == 0) {
			//bone->local *= mat4::Rotation(0, 0, Kore::pi * 0.01f);
		}
	}
	
	for (int i = 0; i < bones.size(); ++i) {
		BoneNode* bone = bones.at(i);
		if (bone->aniTransformations.size() > 0 && frame < bone->aniTransformations.size()) {
			bone->local = bone->aniTransformations.at(frame);
		}
	}
}

void MeshObject::animate(TextureUnit tex) {
	
	// Update bones
	for (int i = 1; i < bones.size(); ++i) updateBone(bones.at(i));
	
	for(int j = 0; j < meshesCount; ++j) {
		int currentBoneCountIndex = 0;	// Iterate over BoneCountArray
		
		Mesh* mesh = meshes.at(j);
		VertexBuffer* vertexBuffer = new VertexBuffer(mesh->numVertices, structure, 0);
		
		// Mesh Vertex Buffer
		float* vertices = vertexBuffer->lock();
		for (int i = 0; i < mesh->numVertices; ++i) {
			vec4 startPos(0, 0, 0, 1);
			vec4 startNormal(0, 0, 0, 1);
			
			// For each vertex belonging to a mesh, the bone count array specifies the number of bones the influence the vertex
			int numOfBones = mesh->boneCountArray[i];
			
			for (int b = 0; b < numOfBones; ++b) {
				vec4 posVec(mesh->vertices[i * 3 + 0], mesh->vertices[i * 3 + 1], mesh->vertices[i * 3 + 2], 1);
				vec4 norVec(mesh->normals[i * 3 + 0], mesh->normals[i * 3 + 1], mesh->normals[i * 3 + 2], 1);
				
				int boneIndex = currentBoneCountIndex + b;
				BoneNode* bone = bones.at(mesh->boneIndices[boneIndex]);
				float boneWeight = mesh->boneWeight[boneIndex];
				
				//startPos += (bone->transform * bone->transformInv * posVec) * boneWeight;
				startPos += (bone->finalTransform * /*bone->transformInv **/ posVec) * boneWeight;
				startNormal += (bone->transform * bone->transformInv * norVec) * boneWeight;
				
				bone->bonePos = startPos;
			}
			
			currentBoneCountIndex += numOfBones;
			
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
		vertexBuffer->unlock();
		
		IndexBuffer* indexBuffer = indexBuffers.at(j);
		Texture* image = images.at(j);
		
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
		ConvertObjects(*openGexDataDescription.GetRootStructure());
		BoneNode* bone = new BoneNode();
		bone->boneName = "";
		ConvertNodes(*openGexDataDescription.GetRootStructure(), *bone);
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

void MeshObject::ConvertNodes(const Structure& rootStructure, BoneNode& parentNode) {
	
	const Structure* structure = rootStructure.GetFirstSubnode();
	while (structure) {
		const OGEX::NodeStructure& nodeStructure = *static_cast<const OGEX::NodeStructure*>(structure);
		
		switch (nodeStructure.GetStructureType()) {
			case OGEX::kStructureNode:
				//return ConvertNode(static_cast<const OGEX::NodeStructure&>(structure));
				
			case OGEX::kStructureBoneNode: {
				BoneNode* bone = ConvertBoneNode(static_cast<const OGEX::BoneNodeStructure&>(nodeStructure));
				bone->parent = &parentNode;
				bones.push_back(bone);
				
				//log(Info, "Bone %s with index %i", bone->boneName, bone->nodeIndex);
				
				children.clear();
				ConvertNodes(*structure, *bone);
				
				children.push_back(bone);

				// Append children to parent
				parentNode.children.insert(parentNode.children.end(), children.begin(), children.end());
				
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
		structure = structure->Next();
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
				
				// Get bone count array
				const Structure *subStructure = skinStructure.GetFirstSubstructure(OGEX::kStructureBoneCountArray);
				const OGEX::BoneCountArrayStructure& boneCountStructure = *static_cast<const OGEX::BoneCountArrayStructure *>(subStructure);
				const unsigned_int16* boneCountArray = boneCountStructure.GetBoneCountArray();
				int boneCount = boneCountStructure.GetVertexCount();
				
				mesh->boneCount = boneCount;
				//mesh->boneCountArray = new int[boneCount];
				//setBoneCountArray(mesh, boneCount, boneCountArray);
				cloneArray(boneCountArray, boneCount, &mesh->boneCountArray);
				//log(Info, "Bone Count %i", boneCount);
				
				// Get weight array
				subStructure = skinStructure.GetFirstSubstructure(OGEX::kStructureBoneWeightArray);
				const OGEX::BoneWeightArrayStructure& weightStructure = *static_cast<const OGEX::BoneWeightArrayStructure *>(subStructure);
				const float* weights = weightStructure.GetBoneWeightArray();
				int weightCount = weightStructure.GetBoneWeightCount();
				
				mesh->weightCount = weightCount;
				//mesh->boneWeight = new float[weightCount];
				//setWeight(mesh, weightCount, weights);
				cloneArray(weights, weightCount, &mesh->boneWeight);
				//log(Info, "Weight Count %i", weightCount);
				
				// Get index array
				subStructure = skinStructure.GetFirstSubstructure(OGEX::kStructureBoneIndexArray);
				const OGEX::BoneIndexArrayStructure& boneIndexStructure = *static_cast<const OGEX::BoneIndexArrayStructure *>(subStructure);
				const unsigned_int16* indices = boneIndexStructure.GetBoneIndexArray();
				int boneIndexCount = boneIndexStructure.GetBoneIndexCount();
				
				mesh->boneIndexCount = boneIndexCount;
				cloneArray(indices, boneIndexCount, &mesh->boneIndices);
				//mesh->boneIndices = new int[boneIndexCount];
				//setBoneIndices(mesh, boneIndexCount, indices);
				//log(Info, "Bone Index Count %i", boneIndexCount);
				
				break;
			}
				
			default:
				break;
		}
		subStructure = subStructure->Next();
	}
	
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
	
	bone->nodeDepth = structure.GetNodeDepth();
	
	//log(Info, "Bone %s with index %i", bone->boneName, bone->nodeIndex);
	
	const Structure *subStructure = structure.GetFirstSubstructure(OGEX::kStructureTransform);
	const OGEX::TransformStructure& transformStructure = *static_cast<const OGEX::TransformStructure *>(subStructure);
	const float* transform = transformStructure.GetTransform();
	bone->transform = getMatrix4x4(transform);
	bone->transformInv = bone->transform.Invert();
	bone->combined = mat4::Identity();
	bone->combinedInv = mat4::Identity();
	bone->local = bone->transform;
	bone->finalTransform = bone->transform;
	
	
	// Get node animation
	subStructure = structure.GetFirstSubstructure(OGEX::kStructureAnimation);
	if (subStructure != nullptr) {
		const OGEX::AnimationStructure& animationStructure = *static_cast<const OGEX::AnimationStructure *>(subStructure);
		const OGEX::TrackStructure& trackStructure = *static_cast<const OGEX::TrackStructure *>(animationStructure.GetFirstSubstructure(OGEX::kStructureTrack));
		const OGEX::TimeStructure* timeStructure = trackStructure.GetTimeStructure();
		const OGEX::KeyStructure* keyStructureTime = timeStructure->GetKeyValueStructure();
		
		const OGEX::ValueStructure* valueStructure = trackStructure.GetValueStructure();
		const OGEX::KeyStructure* keyStructureVal = valueStructure->GetKeyValueStructure();
		const float* data = keyStructureVal->GetData();
		int size = keyStructureVal->GetArraySize();
		int elementCount = keyStructureVal->GetElementCount();
		
		int i = 0;
		while (i < elementCount) {
			float* trans = new float[size];
			
			for (int j = 0; j < size; ++j) {
				trans[j] = data[i];
				i++;
				//log(Info, "pos %i data[%f]", j, trans[j]);
			}
			
			mat4 transMat = getMatrix4x4(trans);
			bone->aniTransformations.push_back(transMat);
		}
	
	}
	return bone;
}
