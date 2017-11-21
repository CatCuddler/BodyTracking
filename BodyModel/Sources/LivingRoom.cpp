#include "LivingRoom.h"

using namespace Kore;
using namespace Kore::Graphics4;

LivingRoom::LivingRoom(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale) : MeshObject(meshFile, textureFile, structure, scale) {

}

void LivingRoom::render(TextureUnit tex, Kore::Graphics4::ConstantLocation mLocation, Kore::Graphics4::ConstantLocation mLocationInverse, ConstantLocation diffuseLocation, ConstantLocation specularLocation, ConstantLocation specularPowerLocation) {
	for (int i = 0; i < meshesCount; ++i) {
		Geometry* geometry = geometries[i];
		mat4 modelMatrix = M * geometry->transform;
		mat4 modelMatrixInverse = modelMatrix.Invert();

		Graphics4::setMatrix(mLocation, modelMatrix);
		Graphics4::setMatrix(mLocationInverse, modelMatrixInverse);

		unsigned int materialIndex = geometry->materialIndex;
		Material* material = findMaterialWithIndex(materialIndex);
		if (material != nullptr) {
			Graphics4::setFloat3(diffuseLocation, material->diffuse);
			Graphics4::setFloat3(specularLocation, material->specular);
			Graphics4::setFloat(specularPowerLocation, material->specular_power);
		}
		else {
			Graphics4::setFloat3(diffuseLocation, vec3(1.0, 1.0, 1.0));
			Graphics4::setFloat3(specularLocation, vec3(1.0, 1.0, 1.0));
			Graphics4::setFloat(specularPowerLocation, 1.0);
		}

		Texture* image = images[i];
		if (image != nullptr) Graphics4::setTexture(tex, image);

		Graphics4::setVertexBuffer(*vertexBuffers[i]);
		Graphics4::setIndexBuffer(*indexBuffers[i]);
		Graphics4::drawIndexedVertices();
	}
}

void LivingRoom::setLights(Kore::Graphics4::ConstantLocation lightCountLocation, Kore::Graphics4::ConstantLocation lightPosLocation) {
	const int lightCount = (int)lights.size();
	for (int i = 0; i < lightCount; ++i) {
		Light* light = lights[i];
		lightPositions[i] = M * light->position;

		if (light->type == 0) {
			lightPositions[i].w() = 0;
		}
		else if (light->type == 1) {
			lightPositions[i].w() = 1;
		}
	}

	Graphics4::setInt(lightCountLocation, lightCount);
	Graphics4::setFloats(lightPosLocation, (float*)lightPositions, lightCount * 4);
}