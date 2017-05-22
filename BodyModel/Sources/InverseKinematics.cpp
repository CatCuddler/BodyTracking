#include "pch.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

#include <Kore/Log.h>

#include <vector>

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) : maxSteps(100), maxError(0.1f), rootIndex(2) {
	bones = boneVec;
	setJointConstraints();
}

bool InverseKinematics::inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone) {
	
	if (!targetBone->initialized) return false;
	if (desiredPos == targetBone->desiredPos) return false;
	
	targetBone->desiredPos = desiredPos;
	
	boneCount = 0;
	BoneNode* bone = targetBone;
	while (bone->nodeIndex != rootIndex) {
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
		InverseKinematics::mat3x jacobianY = calcJacobian(targetBone, Kore::vec4(0, 1, 0, 0));
		InverseKinematics::mat3x jacobianZ = calcJacobian(targetBone, Kore::vec4(0, 0, 1, 0));
		
		// Get pseude inverse
		InverseKinematics::mat3x pseudoInvX = getPsevdoInverse(jacobianX);
		InverseKinematics::mat3x pseudoInvY = getPsevdoInverse(jacobianY);
		InverseKinematics::mat3x pseudoInvZ = getPsevdoInverse(jacobianZ);
		
		mat3x1 V;
		V.Set(0, 0, dif.x()); V.Set(1, 0, dif.y()); V.Set(2, 0, dif.z());
		auto aThetaX = pseudoInvX * V.Transpose();
		auto aThetaY = pseudoInvY * V.Transpose();
		auto aThetaZ = pseudoInvZ * V.Transpose();
		
		std::vector<float> theta;
		for (int i = 0; i < maxBones; ++i) {
			theta.push_back(aThetaX.get(i, 0));
			theta.push_back(aThetaY.get(i, 0));
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
	while (targetBone->nodeIndex != rootIndex) {
		BoneNode* bone = targetBone;
		
		T = relPose(rootIndex, bone->nodeIndex);
		
		Kore::vec4 ai = Kore::vec4(0, 0, 0, 0);
		Kore::vec4 ri = T * Kore::vec4(0, 0, 0, 1);
		
		Kore::vec4 axes = bone->axes;
		if ((axes.x() == 1 && axes.x() == angle.x()) || (axes.y() == 1 && axes.y() == angle.y()) || (axes.z() == 1 && axes.z() == angle.z())) {
			ai = T * angle;
		}
		
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

InverseKinematics::mat3x InverseKinematics::getPsevdoInverse(InverseKinematics::mat3x jacobian) {
	// Left pseudo inverse: (J^T * J ) ^-1 * J^T
	//InverseKinematics::mat6x inv = ((jacobian.Transpose() * jacobian).Invert() * jacobian.Transpose()).Transpose();
	
	// Right pseudo inverse : J^T * (J * J^T) ^−1
	InverseKinematics::mat3x inv = (jacobian.Transpose() * (jacobian * jacobian.Transpose()).Invert()).Transpose();
	
	return inv;
}

void InverseKinematics::applyChanges(std::vector<float> theta, BoneNode* targetBone) {
	
	int i = 0;
	while (targetBone->nodeIndex != rootIndex) {
		BoneNode* bone = targetBone;
		
		float radX = theta.at(i);
		radX = getRadians(radX);
		float radY = theta.at(i+1);
		radY = getRadians(radY);
		float radZ = theta.at(i+2);
		radZ = getRadians(radZ);
		
		Kore::vec4 rot(radX, radY, radZ, 0);
		
		Kore::vec4 axis = bone->axes;
		bone->rotation += rot;
		if (axis.x() == 1) {
			if (bone->rotation.x() < bone->constrain.at(0).x()) bone->rotation.x() = bone->constrain.at(0).x();
			if (bone->rotation.x() > bone->constrain.at(0).y()) bone->rotation.x() = bone->constrain.at(0).y();
		}
		if (axis.y() == 1) {
			if (bone->rotation.y() < bone->constrain.at(1).x()) bone->rotation.y() = bone->constrain.at(1).x();
			if (bone->rotation.y() > bone->constrain.at(1).y()) bone->rotation.y() = bone->constrain.at(1).y();
		}
		if (axis.z() == 1) {
			if (bone->rotation.z() < bone->constrain.at(2).x()) bone->rotation.z() = bone->constrain.at(2).x();
			if (bone->rotation.z() > bone->constrain.at(2).y()) bone->rotation.z() = bone->constrain.at(2).y();
		}
		
		// T * R * S
		Kore::mat4 rotation = quaternionToMatrix(bone->rotation);
		//Kore::mat4 rotation = mat4::Identity().RotationX(bone->rotation.x()) * mat4::Identity().RotationZ(bone->rotation.z());
		bone->local = bone->transform * rotation;
		//Kore::log(Info, "Bone %s -> angle %f %f %f", bone->boneName, bone->rotation.x(), bone->rotation.y(), bone->rotation.z());
		
		targetBone = targetBone->parent;
		i = i + 3;
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

void InverseKinematics::setJointConstraints() {
	BoneNode* nodeLeft;
	BoneNode* nodeRight;
	
	// clavicle
	/*nodeLeft = bones.at(7-1);
	nodeLeft->axes = Kore::vec4(1, 0, 1, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-0.3, 0.3));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0.2));
	
	nodeRight = bones.at(26-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;*/
	
	// upperarm
	nodeLeft = bones.at(8-1);
	nodeLeft->axes = Kore::vec4(1, 0, 1, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-0.3, 1.5));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(-0.4, 0.3));
	
	nodeRight = bones.at(27-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// lowerarm
	nodeLeft = bones.at(9-1);
	nodeLeft->axes = Kore::vec4(1, 1, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-0.3, 1));
	nodeLeft->constrain.push_back(Kore::vec2(-0.05, 0.05));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	
	nodeRight = bones.at(28-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// thigh
	nodeLeft = bones.at(47-1);
	nodeLeft->axes = Kore::vec4(1, 0, 1, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-2, 0.5));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(-1, 0.5));
	
	nodeRight = bones.at(51-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// calf
	nodeLeft = bones.at(48-1);
	nodeLeft->axes = Kore::vec4(1, 0, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(0, 2));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	
	nodeRight = bones.at(52-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
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
