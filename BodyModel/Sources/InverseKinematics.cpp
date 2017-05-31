#include "pch.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

#include <Kore/Log.h>

#include <vector>

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) : maxSteps(10), maxError(0.01f), rootIndex(2) {
	bones = boneVec;
	setJointConstraints();
}

bool InverseKinematics::inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone) {
	
	if (!targetBone->initialized) return false;
	if (desiredPos == targetBone->desiredPos) return false;
	
	//targetBone->desiredPos = desiredPos;
	
	boneCount = 0;
	BoneNode* bone = targetBone;
	while (bone->nodeIndex != rootIndex) {
		//if (bone->interpolating) return false; 
		bone = bone->parent;
		++boneCount;
	}
	
	//Kore::log(Kore::Info, "Bone name %s, Max bone number %i", targetBone->boneName, boneCount);
	if (boneCount > maxBones) {
		Kore::log(Kore::Error, "Increase the max bone number");
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
		
		// Get Pseude Inverse
		InverseKinematics::mat3x pseudoInvX = getPsevdoInverse(jacobianX);
		InverseKinematics::mat3x pseudoInvY = getPsevdoInverse(jacobianY);
		InverseKinematics::mat3x pseudoInvZ = getPsevdoInverse(jacobianZ);
		
		// Calculate the angles
		InverseKinematics::mat3x1 V;
		V.Set(0, 0, dif.x()); V.Set(1, 0, dif.y()); V.Set(2, 0, dif.z());
		InverseKinematics::mat1x aThetaX = pseudoInvX * V.Transpose();
		InverseKinematics::mat1x aThetaY = pseudoInvY * V.Transpose();
		InverseKinematics::mat1x aThetaZ = pseudoInvZ * V.Transpose();
		
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

InverseKinematics::mat3x InverseKinematics::calcJacobian(BoneNode* targetBone, Kore::vec4 rotAxis) {
	
	InverseKinematics::mat3x jacobian;
	
	// Get current position of the end-effector
	Kore::vec4 opn = targetBone->combined * Kore::vec4(0, 0, 0, 1);
	
	int j = 0;
	while (targetBone->nodeIndex != rootIndex) {
		BoneNode* bone = targetBone;
		
		// Get rotation and position vector of the current bone
		Kore::vec4 oaj = Kore::vec4(0, 0, 0, 0);
		Kore::vec4 opj = bone->combined * Kore::vec4(0, 0, 0, 1);
		
		Kore::vec4 axes = bone->axes;
		if ((axes.x() == 1 && axes.x() == rotAxis.x()) || (axes.y() == 1 && axes.y() == rotAxis.y()) || (axes.z() == 1 && axes.z() == rotAxis.z())) {
			oaj = bone->combined * rotAxis;
		}
		
		// Calculate cross product
		Kore::vec4 cross = oaj.cross(opn - opj);
		
		jacobian[j][0] = cross.x();
		jacobian[j][1] = cross.y();
		jacobian[j][2] = cross.z();
		
		targetBone = targetBone->parent;
		++j;
	}
	
	return jacobian;
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
		float radY = theta.at(i+1);
		float radZ = theta.at(i+2);
		/*radX = getRadians(radX);
		radY = getRadians(radY);
		radZ = getRadians(radZ);*/
		
		bone->rotation.x() += radX;
		bone->rotation.y() += radY;
		bone->rotation.z() += radZ;
		
		Kore::vec4 axis = bone->axes;
		if (axis.x() == 1) clampValue(bone->constrain.at(0).x(), bone->constrain.at(0).y(), &bone->rotation.x());
		if (axis.y() == 1) clampValue(bone->constrain.at(1).x(), bone->constrain.at(1).y(), &bone->rotation.y());
		if (axis.z() == 1) clampValue(bone->constrain.at(2).x(), bone->constrain.at(2).y(), &bone->rotation.z());
		
		Kore::Quaternion quat;
		eulerToQuat(getRadians(bone->rotation.x()), getRadians(bone->rotation.y()), getRadians(bone->rotation.z()), &quat);
		bone->desQuaternion = quat;
		
		// T * R * S
		quat.normalize();
		Kore::mat4 rotMat = quat.matrix().Transpose();
		//Kore::mat4 rotMat = mat4::Rotation(bone->rotation.z(), bone->rotation.y(), bone->rotation.x());
		bone->local = bone->transform * rotMat;
		//Kore::log(Info, "Bone %s -> angle %f %f %f", bone->boneName, bone->rotation.x(), bone->rotation.y(), bone->rotation.z());
		
		targetBone = targetBone->parent;
		i = i + 3;
	}
	
}

void InverseKinematics::clampValue(float minVal, float maxVal, float* value) {
	if (*value < minVal) *value = minVal;
	else if (*value > maxVal) *value = maxVal;
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

void InverseKinematics::eulerToQuat(float roll, float pitch, float yaw, Kore::Quaternion* quat) {
	float cr, cp, cy, sr, sp, sy, cpcy, spsy;
	// calculate trig identities
	cr = Kore::cos(roll/2);
	cp = Kore::cos(pitch/2);
	cy = Kore::cos(yaw/2);
	sr = Kore::sin(roll/2);
	sp = Kore::sin(pitch/2);
	sy = Kore::sin(yaw/2);
	cpcy = cp * cy;
	spsy = sp * sy;
	quat->w = cr * cpcy + sr * spsy;
	quat->x = sr * cpcy - cr * spsy;
	quat->y = cr * sp * cy + sr * cp * sy;
	quat->z = cr * cp * sy - sr * sp * cy;
}

void InverseKinematics::setJointConstraints() {
	BoneNode* nodeLeft;
	BoneNode* nodeRight;
	
	// clavicle
	/*nodeLeft = bones.at(7-1);
	nodeLeft->axes = Kore::vec4(0, 0, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-0.05, 0.05));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	
	nodeRight = bones.at(26-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;*/
	
	// upperarm
	nodeLeft = bones.at(8-1);
	nodeLeft->axes = Kore::vec4(1, 1, 1, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-60, 130));
	nodeLeft->constrain.push_back(Kore::vec2(-45, 45));
	nodeLeft->constrain.push_back(Kore::vec2(-20, 60));
	
	nodeRight = bones.at(27-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// lowerarm
	nodeLeft = bones.at(9-1);
	nodeLeft->axes = Kore::vec4(1, 1, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-30, 90));
	nodeLeft->constrain.push_back(Kore::vec2(-45, 45));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	
	nodeRight = bones.at(28-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// thigh
	nodeLeft = bones.at(47-1);
	nodeLeft->axes = Kore::vec4(1, 0, 1, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-90, 60));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(-80, 80));
	
	nodeRight = bones.at(51-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// calf
	nodeLeft = bones.at(48-1);
	nodeLeft->axes = Kore::vec4(1, 0, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-10, 150));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	
	nodeRight = bones.at(52-1);
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
}
