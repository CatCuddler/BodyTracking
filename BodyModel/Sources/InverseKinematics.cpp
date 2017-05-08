#include "pch.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

#include <Kore/Log.h>

#include <vector>

namespace Kore {
	int lastIndexCount = 3;
	int* lastIndex = new int[lastIndexCount];
}

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) : maxSteps(100), maxError(0.1f), rootIndex(2) {
	lastIndex = new int[2];
	lastIndex[0] = 7;	// for left hand
	lastIndex[1] = 26;	// for right hand
	lastIndex[2] = 3;	// for both feet
	
	bones = boneVec;
}

bool InverseKinematics::inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone) {
	
	if (!targetBone->initialized) return false;
	if (desiredPos == targetBone->desiredPos) return false;
	
	targetBone->desiredPos = desiredPos;
	
	boneCount = 0;
	BoneNode* bone = targetBone;
	while (!checkBoneIndex(bone->nodeIndex)) {
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
		InverseKinematics::mat3x jacobianX = calcJacobian(targetBone, Kore::vec4(1, 0, 0, 0));
		InverseKinematics::mat3x jacobianZ = calcJacobian(targetBone, Kore::vec4(0, 0, 1, 0));
		
		// Get pseude inverse
		InverseKinematics::mat3x pseudoInvX = getPsevdoInverse(jacobianX);
		InverseKinematics::mat3x pseudoInvZ = getPsevdoInverse(jacobianZ);
		
		mat3x1 V;
		V.Set(0, 0, dif.x()); V.Set(1, 0, dif.y()); V.Set(2, 0, dif.z());
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

InverseKinematics::mat3x InverseKinematics::calcJacobian(BoneNode* targetBone, Kore::vec4 angle) {
	
	InverseKinematics::mat3x jacobian;
	
	Kore::mat4 T = relPose(rootIndex, targetBone->nodeIndex);
	Kore::vec4 orn = T * Kore::vec4(0, 0, 0, 1);
	
	int i = 0;
	while (!checkBoneIndex(targetBone->nodeIndex)) {
		BoneNode* bone = targetBone;
		
		T = relPose(rootIndex, bone->nodeIndex);
		
		Kore::vec4 ai = T * angle;
		Kore::vec4 ri = T * Kore::vec4(0, 0, 0, 1);
		
		Kore::vec4 cross = ai.cross(orn - ri);
		
		jacobian[i][0] = cross.x();
		jacobian[i][1] = cross.y();
		jacobian[i][2] = cross.z();
		
		if (bone->nodeIndex == 28 || bone->nodeIndex == 7) {
			// Clavicle
			jacobian[i][0] = 0;
			jacobian[i][1] = 0;
			jacobian[i][2] = 0;
		}
		
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

InverseKinematics::mat3x InverseKinematics::getPsevdoInverse(InverseKinematics::mat3x jacobian) {
	// Left pseudo inverse: (J^T * J ) ^-1 * J^T
	//InverseKinematics::mat6x inv = ((jacobian.Transpose() * jacobian).Invert() * jacobian.Transpose()).Transpose();
	
	// Right pseudo inverse : J^T * (J * J^T) ^−1
	InverseKinematics::mat3x inv = (jacobian.Transpose() * (jacobian * jacobian.Transpose()).Invert()).Transpose();
	
	return inv;
}

void InverseKinematics::applyChanges(std::vector<float> theta, BoneNode* targetBone) {
	
	int i = 0;
	while (!checkBoneIndex(targetBone->nodeIndex)) {
		BoneNode* bone = targetBone;
		
		float radX = theta.at(i);
		radX = getRadians(radX);
		float radZ = theta.at(i+1);
		radZ = getRadians(radZ);
		
		Kore::vec4 rot(radX, 0, radZ, 0);
		vec3 newRot = bone->rotation + rot;
		
		// Constraints
		if (bone->nodeIndex == 47 ) {
			// Thigh
			//if (radX < 0) radX = 0;
			//if (radX > 2) radX = 2;
			//if (newRot.z() > 1) rot.z() = 0.0;
		}
		else if (bone->nodeIndex == 48) {
			// Knee
			if (newRot.x() < -0.1) rot.x() = 0;
			rot.z() = 0;
		}
		
		else if (bone->nodeIndex == 27) {
			// Upperarm
			//rot.x() = 0;
			//if (newRot.z() < -0.4) rot.z() = 0;
		}
		else if (bone->nodeIndex == 28) {
			// Lowerarm
			if (newRot.x() < -0.1) rot.x() = 0;
			//rot.z() = 0;
		}
		bone->rotation += rot;
		
		// T * R * S
		Kore::mat4 rotation = quaternionToMatrix(bone->rotation);
		//Kore::mat4 rotation = mat4::Identity().RotationX(bone->rotation.x()) * mat4::Identity().RotationZ(bone->rotation.z());
		bone->local = bone->transform * rotation;
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

Kore::mat4 InverseKinematics::quaternionToMatrix(Kore::vec4 quat) {
	Kore::mat4 rot = Kore::mat4::Identity();
	
	float qx = quat.x();
	float qy = quat.y();
	float qz = quat.z();
	float qw = quat.w();
	
	vec3 term = vec3(1 - 2*qy*qy - 2*qz*qz, 2*qx*qy - 2*qz*qw, 2*qx*qz + 2*qy*qw);
	term = term.normalize();
	rot.Set(0, 0, term.x());
	rot.Set(0, 1, term.y());
	rot.Set(0, 2, term.z());
	
	term = vec3(2*qx*qy + 2*qz*qw, 1 - 2*qx*qx - 2*qz*qz, 2*qy*qz - 2*qx*qw);
	term = term.normalize();
	rot.Set(1, 0, term.x());
	rot.Set(1, 1, term.y());
	rot.Set(1, 2, term.z());
	
	term = vec3(2*qx*qz - 2*qy*qw, 2*qy*qz + 2*qx*qw, 1 - 2*qx*qx - 2*qy*qy);
	term = term.normalize();
	rot.Set(2, 0, term.x());
	rot.Set(2, 1, term.y());
	rot.Set(2, 2, term.z());
	
	return rot;
}

float InverseKinematics::getRadians(float degree) {
	const double halfC = Kore::pi / 180.0f;
	return degree * halfC;
}

bool InverseKinematics::checkBoneIndex(int boneIndex) {
	for(int i = 0; i < lastIndexCount; ++i) {
		if (boneIndex == lastIndex[i]) return true;
	}
	return false;
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
