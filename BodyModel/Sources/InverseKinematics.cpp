#include "pch.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

#include <Kore/Log.h>

#include <vector>

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) : maxSteps(1), maxError(0.1f) {
	bones = boneVec;
}

bool InverseKinematics::inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone) {
	
	if (!targetBone->initialized) return false;
	
	boneCount = 0;
	BoneNode* bone = targetBone;
	while (bone->nodeIndex > lastIndex) {
		bone = bone->parent;
		++boneCount;
	}
	
	for (int i = 0; i < maxSteps; ++i) {
		
		// Calculate error between desired position and actual position of the end effector
		Kore::vec4 currentPos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
		Kore::vec4 dif = desiredPos - currentPos;
		
		log(Info, "Current Pos: (%f %f %f), Desired Pos: (%f %f %f)", currentPos.x(), currentPos.y(), currentPos.z(), desiredPos.x(), desiredPos.y(), desiredPos.z());
		
		float err = dif.getLength();
		if (err < maxError) {
			return true;
		}
		
		// Calculate Jacobi Matrix
		InverseKinematics::mat6x jacobian = calcJacobian(targetBone);
		
		// Get pseude inverse
		InverseKinematics::mat6x pseudoInv = getPsevdoInverse(jacobian);
		//Kore::vec6 theta = pseudoInv * Kore::vec6(dif.x(), dif.y(), dif.z(), 0, 0, 0);
		
		mat6x1 V;
		V.Set(0, 0, dif.x()); V.Set(1, 0, dif.y()); V.Set(2, 0, dif.z());
		V.Set(3, 0, 0); V.Set(4, 0, 0); V.Set(5, 0, 0);
		auto aTheta = pseudoInv * V.Transpose();
		
		std::vector<float> theta;
		for (int i = 0; i < maxBones; ++i) {
			theta.push_back(aTheta.get(i, 0));
		}
		
		
		applyChanges(theta, targetBone);
		//updateBonePosition(targetBone);
		for (int i = 0; i < bones.size(); ++i) updateBonePosition(bones.at(i));
	}
	return false;
}

InverseKinematics::mat6x InverseKinematics::calcJacobian(BoneNode* targetBone) {
	
	InverseKinematics::mat6x jacobian;
	
	Kore::vec4 orn = targetBone->combined * Kore::vec4(0, 0, 0, 1);
	
	bool isEndEffector = true;
	
	int i = 0;
	while (targetBone->nodeIndex > lastIndex) {
		BoneNode* bone = targetBone;
		
		Kore::vec4 xi = bone->combined * Kore::vec4(1, 0, 0, 0);
		//Kore::vec4 zi = bone->combined * Kore::vec4(0, 0, 1, 0);
		Kore::vec4 ri = bone->combined * Kore::vec4(0, 0, 0, 1);
		
		if (isEndEffector) {
			xi = vec4(1, 0, 0, 0);
			//zi = vec4(0, 0, 1, 0);
			ri = vec4(0, 0, 0, 1);
			isEndEffector = false;
		}
		
		//Kore::log(Info, "Bone %s", bone->boneName);
		
		Kore::vec4 cross = xi.cross(orn - ri);
		
		jacobian[i][0] = cross.x();
		jacobian[i][1] = cross.y();
		jacobian[i][2] = cross.z();
		
		jacobian[i][3] = xi.x();
		jacobian[i][4] = xi.y();
		jacobian[i][5] = xi.z();
		
		targetBone = targetBone->parent;
		++i;
	}
	
	return jacobian;
}

InverseKinematics::mat6x InverseKinematics::getPsevdoInverse(InverseKinematics::mat6x jacobian) {
	
	// Left pseudo inverse: (J^T * J ) ^-1 * J^T
	//InverseKinematics::mat6x inv = ((jacobian.Transpose() * jacobian).Invert() * jacobian.Transpose()).Transpose();
	
	// Right pseudo inverse : J^T * (J * J^T) ^−1
	InverseKinematics::mat6x inv = (jacobian.Transpose() * (jacobian * jacobian.Transpose()).Invert()).Transpose();
	
	return inv;
}

void InverseKinematics::applyChanges(std::vector<float> theta, BoneNode* targetBone) {
	
	int i = 0;
	while (targetBone->nodeIndex > lastIndex) {
		BoneNode* bone = targetBone;
		
		Kore::vec4 currentPos = targetBone->local * Kore::vec4(0, 0, 0, 1);
		float angle = theta.at(boneCount-i-1);
		angle = getRadians(angle);
		
		//T * R * S
		
		Kore::log(Info, "Bone %s -> angle %f", bone->boneName, angle);
		bone->local *= bone->local.RotationZ(angle);
		//bone->local = bone->local.Translation(currentPos.x(), currentPos.y(), currentPos.z()) * bone->local.RotationZ(angle);
		
		//Kore::vec4 zAngle = targetBone->combined * Kore::vec4(0, 0, 1, 0);
		//Kore::mat4 test = mat4::RotationZ(angle) * targetBone->combined;
		
		
		targetBone = targetBone->parent;
		++i;
	}
	
}

void InverseKinematics::updateBonePosition(BoneNode *targetBone) {
	
	/*while (targetBone->nodeIndex > lastIndex) {
		Kore::vec4 oldPos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
		
		targetBone->combined = targetBone->parent->combined * targetBone->local;
		
		Kore::vec4 newPos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
		Kore::log(Info, "1Bone %s -> oldPos (%f %f %f) newPos (%f %f %f)", targetBone->boneName, oldPos.x(), oldPos.y(), oldPos.z(), newPos.x(), newPos.y(), newPos.z());
		
		//targetBone->finalTransform = targetBone->combined * targetBone->combinedInv;
		
		targetBone = targetBone->parent;
	}*/
	
	Kore::vec4 oldPos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
	
	targetBone->combined = targetBone->parent->combined * targetBone->local;
	
	Kore::vec4 newPos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
	if (targetBone->nodeIndex > 50 && targetBone->nodeIndex < 55)
		Kore::log(Info, "Bone %s -> oldPos (%f %f %f) newPos (%f %f %f)", targetBone->boneName, oldPos.x(), oldPos.y(), oldPos.z(), newPos.x(), newPos.y(), newPos.z());
	
	targetBone->finalTransform = targetBone->combined * targetBone->combinedInv;
}

float InverseKinematics::getRadians(float degree) {
	const double halfC = Kore::pi / 180.0f;
	return degree * halfC;
}

Kore::vec3 InverseKinematics::getAngles(Kore::mat4 rot) {
	float roll = Kore::atan2(rot.get(2,1), rot.get(2,2));
	float pitch = Kore::atan2(-rot.get(2,0), sqrt(rot.get(2,1)*rot.get(2,1) + rot.get(2,2)*rot.get(2,2)));
	float yaw = Kore::atan2(rot.get(1,0), rot.get(0,0));
	
	/*float s = 0.5f * (rMat.Trace() + 1);
	float roll = s * (rMat.get(2, 1) - rMat.get(1, 2));
	float pitch = s * (rMat.get(0, 2) - rMat.get(2, 0));
	float yaw = s * (rMat.get(1, 0) - rMat.get(0, 1));*/
	
	return vec3(roll, pitch, yaw);
}
