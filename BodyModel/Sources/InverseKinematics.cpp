#include "pch.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

#include <Kore/Log.h>

#include <vector>

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) : maxSteps(100), maxError(0.1f), rootIndex(2) {
	bones = boneVec;
}

bool InverseKinematics::inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone) {
	
	if (!targetBone->initialized) return false;
	if (desiredPos == targetBone->desiredPos) return false;
	
	targetBone->desiredPos = desiredPos;
	
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
		
		//Kore::log(Info, "It: %i, Current Pos: (%f %f %f), Desired Pos: (%f %f %f)", i, currentPos.x(), currentPos.y(), currentPos.z(), desiredPos.x(), desiredPos.y(), desiredPos.z());
		
		float error = dif.getLength();
		if (error < maxError) {
			return true;
		}
		
		// Calculate Jacobi Matrix
		InverseKinematics::mat6x jacobianX = calcJacobian(targetBone, Kore::vec4(1, 0, 0, 0));
		InverseKinematics::mat6x jacobianZ = calcJacobian(targetBone, Kore::vec4(0, 0, 1, 0));
		
		// Get pseude inverse
		InverseKinematics::mat6x pseudoInvX = getPsevdoInverse(jacobianX);
		InverseKinematics::mat6x pseudoInvZ = getPsevdoInverse(jacobianZ);
		
		mat6x1 V;
		V.Set(0, 0, dif.x()); V.Set(1, 0, dif.y()); V.Set(2, 0, dif.z());
		//V.Set(3, 0, 0); V.Set(4, 0, 0); V.Set(5, 0, 0);
		auto aThetaX = pseudoInvX * V.Transpose();
		auto aThetaZ = pseudoInvZ * V.Transpose();
		
		std::vector<float> theta;
		for (int i = 0; i < maxBones; ++i) {
			theta.push_back(aThetaX.get(i, 0));
			theta.push_back(aThetaZ.get(i, 0));
		}
		
		applyChanges(theta, targetBone);
		for (int i = 0; i < bones.size(); ++i) updateBonePosition(bones.at(i));
	}
	return false;
}

InverseKinematics::mat6x InverseKinematics::calcJacobian(BoneNode* targetBone, Kore::vec4 angle) {
	
	InverseKinematics::mat6x jacobian;
	
	Kore::mat4 T = relPose(rootIndex, targetBone->nodeIndex);
	Kore::vec4 orn = T * Kore::vec4(0, 0, 0, 1);
	
	int i = 0;
	while (targetBone->nodeIndex > lastIndex) {
		BoneNode* bone = targetBone;
		
		T = relPose(rootIndex, bone->nodeIndex);
		
		Kore::vec4 ai = T * angle;
		Kore::vec4 ri = T * Kore::vec4(0, 0, 0, 1);
		
		Kore::vec4 cross = ai.cross(orn - ri);
		
		jacobian[i][0] = cross.x();
		jacobian[i][1] = cross.y();
		jacobian[i][2] = cross.z();
		
		targetBone = targetBone->parent;
		++i;
	}
	
	return jacobian;
}

Kore::mat4 InverseKinematics::relPose(int i, int j) {
	
	Kore::mat4 oTi, oTj;
	if (i == rootIndex) oTi = mat4::Identity();
	else oTi = bones.at(i-1)->combined;
	
	if (j == rootIndex) oTj = mat4::Identity();
	else oTj = bones.at(j-1)->combined;
	
	Kore::mat4 result = oTi.Invert() * oTj;
	return result;
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
		
		float radX = theta.at(i);
		radX = getRadians(radX);
		float radZ = theta.at(i+1);
		radZ = getRadians(radZ);
		
		Kore::vec4 rot(radX, 0, radZ, 0);
		
		// Constraints
		if (bone->nodeIndex == 47 || bone->nodeIndex == 51) {
			// Thigh
			//if (radX < 0) radX = 0;
			//if (radX > 2) radX = 2;
			//if (bone->rotation.z() > 0.3) rot.z() = 0.0;
		}
		if (bone->nodeIndex == 48 || bone->nodeIndex == 52) {
			// Knee
			if (bone->rotation.x() < -0.1) rot.x() = 0;
			rot.z() = 0;
		}
		bone->rotation += rot;

		bone->local *= bone->local.RotationX(rot.x()) * bone->local.RotationZ(rot.z());;
		//Kore::log(Info, "Bone %s -> angle %f %f %f", bone->boneName, bone->rotation.x(), bone->rotation.y(), bone->rotation.z());
		
		targetBone = targetBone->parent;
		i = i + 2;
	}
	
}

void InverseKinematics::updateBonePosition(BoneNode *targetBone) {
	//Kore::vec4 oldPos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
	
	targetBone->combined = targetBone->parent->combined * targetBone->local;
	
	//Kore::vec4 newPos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
	//Kore::log(Info, "Bone %s -> oldPos (%f %f %f) newPos (%f %f %f)", targetBone->boneName, oldPos.x(), oldPos.y(), oldPos.z(), newPos.x(), newPos.y(), newPos.z());
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
