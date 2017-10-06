#include "pch.h"
#include "MeshObject.h"

#include <Kore/Graphics1/Color.h>
#include <Kore/IO/FileReader.h>
#include <Kore/Log.h>

#include <sstream>
#include <algorithm>

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
	
	void setVertexFromMesh(float* vertices, Mesh* mesh, float vertexScale = 1.0, float texScaleX = 1.0, float texScaleY = 1.0) {
		for (int i = 0; i < mesh->numVertices; ++i) {
			// position
			vertices[i * 8 + 0] = mesh->vertices[i * 3 + 0] * vertexScale;
			vertices[i * 8 + 1] = mesh->vertices[i * 3 + 1] * vertexScale;
			vertices[i * 8 + 2] = mesh->vertices[i * 3 + 2] * vertexScale;
			// texCoord
			vertices[i * 8 + 3] = mesh->texcoord[i * 2 + 0] * texScaleX;
			vertices[i * 8 + 4] = (1.0f - mesh->texcoord[i * 2 + 1]) * texScaleY;
			// normal
			vertices[i * 8 + 5] = mesh->normals[i * 3 + 0];
			vertices[i * 8 + 6] = mesh->normals[i * 3 + 1];
			vertices[i * 8 + 7] = mesh->normals[i * 3 + 2];
			
			//log(Info, "%f %f %f %f %f %f %f %f", vertices[i * 8 + 0], vertices[i * 8 + 1], vertices[i * 8 + 2], vertices[i * 8 + 3], vertices[i * 8 + 4], vertices[i * 8 + 5], vertices[i * 8 + 6], vertices[i * 8 + 7]);
		}
	}
	
	void setIndexFromMesh(int* indices, Mesh* mesh) {
		for (int i = 0; i < mesh->numFaces * 3; ++i) {
			indices[i] = mesh->indices[i];
			
			//log(Info, "%i", indices[i]);
		}

	}
	
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
	
	unsigned int getIndexFromString(const char* name, int ignore) {
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
	
	vec3 getVector3(const float* vector) {
		vec3 vec = vec4(0, 0, 0);
		vec.x() = vector[0];
		vec.y() = vector[1];
		vec.z() = vector[2];
		return vec;
	}
	
}

MeshObject::MeshObject(const char* meshFile, const char* textureFile, const VertexStructure& structure, float scale) : textureDir(textureFile), structure(structure), scale(scale), M(mat4::Identity()) {
	
	LoadObj(meshFile);
	
	meshesCount = meshes.size();
	log(Info, "Meshes length %i, geometry length %i, material length %i", meshesCount, geometries.size(), materials.size());
	
	std::sort(meshes.begin(), meshes.end(), CompareMesh());
	std::sort(geometries.begin(), geometries.end(), CompareGeometry());
	std::sort(materials.begin(), materials.end(), CompareMaterials());
	
	vertexBuffers = new VertexBuffer*[meshesCount];
	indexBuffers = new IndexBuffer*[meshesCount];
	images = new Texture*[meshesCount];
	for(int j = 0; j < meshesCount; ++j) {
		Mesh* mesh = meshes[j];
		
		Geometry* geometry = geometries[j];
		unsigned int materialIndex = geometry->materialIndex;
		Material* material = findMaterialWithIndex(materialIndex);
		images[j] = nullptr;
		if (material != nullptr && material->textureName != nullptr) {
			char temp[200];
			std::strcpy(temp, textureDir);
			std::strcat(temp, material->textureName);
			log(Info, "Load Texture %s", temp);
			Texture* image = new Texture(temp, true);
			images[j] = image;
		}
		
		// Mesh Vertex Buffer
		vertexBuffers[j] = new VertexBuffer(mesh->numVertices, structure, 0);
		float* vertices = vertexBuffers[j]->lock();
		setVertexFromMesh(vertices, mesh, scale, material->texScaleX, material->texScaleY);
		vertexBuffers[j]->unlock();
		
		// Mesh Index Buffer
		indexBuffers[j] = new IndexBuffer(mesh->numFaces * 3);
		int* indices = indexBuffers[j]->lock();
		setIndexFromMesh(indices, mesh);
		indexBuffers[j]->unlock();
		
	}
	
}

void MeshObject::render(TextureUnit tex) {
	for (int i = 0; i < meshesCount; ++i) {		
		Texture* image = images[i];
		Graphics4::setTexture(tex, image);
		
		Graphics4::setVertexBuffer(*vertexBuffers[i]);
		Graphics4::setIndexBuffer(*indexBuffers[i]);
		Graphics4::drawIndexedVertices();
	}
}

void MeshObject::LoadObj(const char* filename) {
	FileReader fileReader(filename, FileReader::Asset);
	void* data = fileReader.readAll();
	int size = fileReader.size();
	char* buffer = new char[size + 1];
	for (int i = 0; i < size; ++i) buffer[i] = reinterpret_cast<char*>(data)[i];
	buffer[size] = 0;
	
	OGEX::OpenGexDataDescription openGexDataDescription;
	DataResult result = openGexDataDescription.ProcessText(buffer);
	if (result == kDataOkay) {
		ConvertObjects(*openGexDataDescription.GetRootStructure());
		BoneNode* bone = new BoneNode();
		ConvertNodes(*openGexDataDescription.GetRootStructure(), *bone);
	} else {
		log(Info, "Failed to load OpenGEX file");
	}
	
	delete[] buffer;
}

BoneNode* MeshObject::getBoneWithIndex(int boneIndex) const {
	BoneNode* bone = bones[boneIndex - 1];
	return bone;
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
				
				ConvertNodes(*structure, *bone);
				
				break;
			}
				
			case OGEX::kStructureGeometryNode: {
				Geometry* geometry = ConvertGeometryNode(static_cast<const OGEX::GeometryNodeStructure&>(nodeStructure));
				geometries.push_back(geometry);
				break;
			}
				
			case OGEX::kStructureLightNode: {
				Light* light = ConvertLightNode(static_cast<const OGEX::LightNodeStructure&>(nodeStructure));
				lights.push_back(light);
				break;
			}
				
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
	
	geometry->name = structure.GetNodeName();
	//log(Info, "Geometry name %s", name);
	
	const Structure *subStructure = structure.GetFirstSubnode();
	while (subStructure) {
		switch (subStructure->GetStructureType()) {
			case OGEX::kStructureTransform: {
				const OGEX::TransformStructure& transformStructure = *static_cast<const OGEX::TransformStructure *>(subStructure);
				const float* transform = transformStructure.GetTransform();
				geometry->transform = getMatrix4x4(transform);
				
				break;
			}
				
			case OGEX::kStructureMaterialRef: {
				const OGEX::MaterialRefStructure& materialRefStructure = *static_cast<const OGEX::MaterialRefStructure *>(subStructure);
				const Structure* subSubStructure = materialRefStructure.GetTargetStructure();
				
				geometry->materialRef = subSubStructure->GetStructureName();				
				geometry->materialIndex = getIndexFromString(geometry->materialRef, 8);
				break;
			}
				
			case OGEX::kStructureObjectRef: {
				const OGEX::ObjectRefStructure& objectRefStructure = *static_cast<const OGEX::ObjectRefStructure *>(subStructure);
				const Structure* subSubStructure = objectRefStructure.GetTargetStructure();
				
				geometry->objectRef = subSubStructure->GetStructureName();
				geometry->geometryIndex = getIndexFromString(geometry->objectRef, 8);

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
	
	material->materialName = materialStructure.GetStructureName();
	material->materialIndex = getIndexFromString(material->materialName, 8);
	//log(Info, "Material name %s, index %i", material->materialName, material->materialIndex);
	
	const Structure* subStructure = materialStructure.GetFirstSubnode();
	while (subStructure) {
		
		switch (subStructure->GetStructureType()) {
				
			case OGEX::kStructureColor: {
				const OGEX::ColorStructure& colorStructure = *static_cast<const OGEX::ColorStructure *>(subStructure);
				
				const String& attrib = colorStructure.GetAttribString();
				if (attrib == "diffuse") {
					const float* diffuse = colorStructure.GetColor();
					material->diffuse = getVector3(diffuse);
				} else if (attrib == "specular") {
					const float* specular = colorStructure.GetColor();
					material->specular = getVector3(specular);
				}

				break;
			}
				
			case OGEX::kStructureParam: {
				const OGEX::ParamStructure& paramStructure = *static_cast<const OGEX::ParamStructure *>(subStructure);
				
				const String& attrib = paramStructure.GetAttribString();
				if (attrib == "specular_power") {
					material->specular_power = paramStructure.GetParam();
				}
				
				break;
			}
				
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
				
				// Get texture scale factor
				const Structure *subStructure = textureStructure.GetFirstSubstructure(OGEX::kStructureTransform);
				if (subStructure != nullptr) {
					const OGEX::TransformStructure& transformStructure = *static_cast<const OGEX::TransformStructure *>(subStructure);
					Kore::mat4 transform = getMatrix4x4(transformStructure.GetTransform());
					
					material->texScaleX = transform.get(0, 0);
					material->texScaleY = transform.get(1, 1);
				} else {
					material->texScaleX = 1;
					material->texScaleY = 1;
				}
				
				break;
			}
				
			default:
				break;
		}
		
		subStructure = subStructure->Next();
	}
	
	//log(Info, "Texture name %s, color (%f %f %f)", material->materialName, material->color.x(), material->color.y(), material->color.z());
	
	return material;
}

Material* MeshObject::findMaterialWithIndex(const int index) {
	for (int i = 0; i < materials.size(); ++i) {
		Material* mat = materials[i];
		if (mat->materialIndex == index) {
			return mat;
		}
	}
	return nullptr;
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
	bone->local = bone->transform;
	
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

Light* MeshObject::ConvertLightNode(const OGEX::LightNodeStructure& structure) {
	Light* light = new Light();
	
	const char* name = structure.GetNodeName();
	light->name = name;
	
	std::string lightName(name);
	if (lightName.compare("Spot") == 0) {
		light->type = 0;
	} else if (lightName.compare("Point") == 0) {
		light->type = 1;
	}
	
	//log(Info, "Light name %s, type %i", name, light->type);
	
	const Structure *subStructure = structure.GetFirstSubnode();
	while (subStructure) {
		switch (subStructure->GetStructureType()) {
			case OGEX::kStructureTransform: {
				const OGEX::TransformStructure& transformStructure = *static_cast<const OGEX::TransformStructure *>(subStructure);
				const float* transform = transformStructure.GetTransform();
				mat4 lightPos = getMatrix4x4(transform);
				light->position = lightPos * vec4(0, 0, 0, 1);
				
				break;
			}
				
			default:
				break;
		}
		subStructure = subStructure->Next();
	}
	
	return light;
}

void MeshObject::setScale(float scaleFactor) {
	// Scale root bone
	BoneNode* root = bones[0];
	
	mat4 scaleMat = mat4::Identity();
	scaleMat.Set(3, 3, 1.0/scaleFactor);
	
	root->transform = root->transform * scaleMat; //T * R * S
	root->local = root->transform;
	
	scale = scaleFactor;
}



