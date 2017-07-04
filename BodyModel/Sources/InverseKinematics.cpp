#include "pch.h"
#include "RotationUtility.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

#include <Kore/Log.h>

#include <vector>

namespace {

	Kore::vec4 rotMatToRotVec(Kore::mat4 mat) {
		float eps = 0.001;
		
		float th = Kore::acos(0.5 * (mat[0][0] + mat[1][1] + mat[2][2] - 1));
		
		Kore::vec4 n;
		if (Kore::abs(th) < eps) {
			n = vec4(0, 0, 0, 0);
		} else {
			n = 1 / (2 * Kore::sin(th)) * vec4(mat[1][2] - mat[2][1], mat[2][0] - mat[0][2], mat[0][1] - mat[1][0]);
		}
		
		Kore::vec4 phi = th * n;
		return phi;
	}
}

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec, int maxSteps) : maxSteps(maxSteps), maxError(0.01f), rootIndex(2) {
	bones = boneVec;
	setJointConstraints();
}

bool InverseKinematics::inverseKinematics(BoneNode* targetBone, Kore::vec4 desiredPosition, Kore::Quaternion desiredRotation) {

	if (!targetBone->initialized) return false;
	
	boneCount = 0;
	BoneNode* bone = targetBone;
	while (bone->nodeIndex != rootIndex) {
		bone = bone->parent;
		++boneCount;
	}
	
	//Kore::log(Kore::Info, "Bone name %s, Max bone number %i", targetBone->boneName, boneCount);
	if (boneCount > maxBones) {
		Kore::log(Kore::Error, "Increase the max bone number");
	}
	
	for (int i = 0; i < maxSteps; ++i) {
		//log(Info, "Iteration %i", i);
		
		// Calculate error between desired position and actual position of the end effector
		Kore::vec4 currentPosition = targetBone->combined * Kore::vec4(0, 0, 0, 1);
		Kore::vec4 diffPos = desiredPosition - currentPosition;
		
		// Calculate error between deisred rotation and actual rotation
		Kore::Quaternion desQuat = desiredRotation;
		Kore::Quaternion curQuat = targetBone->quaternion;
		Kore::mat4 rot_err = desQuat.matrix() * curQuat.matrix().Invert();
		Kore::vec3 diffRot = rotMatToRotVec(rot_err);
		//Kore::log(Kore::Info, "%f %f %f", diffRot.x(), diffRot.y(), diffRot.z());
		
		/*Kore::Quaternion diff = desQuat * curQuat.invert();
		Kore::RotationUtility::quatToEuler(&diff, &diffRot.x(), &diffRot.y(), &diffRot.z());
		diffRot.x() = 0;
		Kore::log(Kore::Info, "%f %f %f", diffRot.x(), diffRot.y(), diffRot.z());*/
		
		
		//Kore::log(Info, "It: %i, Current Pos: (%f %f %f), Desired Pos: (%f %f %f)", i, currentPos.x(), currentPos.y(), currentPos.z(), desiredPos.x(), desiredPos.y(), desiredPos.z());
		
		float error = diffPos.getLength();
		//log(Info, "error %f", error);
		if (error < maxError) {
			//Kore::log(Kore::Info, "Max it %i", i);
			return true;
		}

		// Calculate Jacobi Matrix
		InverseKinematics::mat6x jacobianX = calcJacobian(targetBone, Kore::vec4(1, 0, 0, 0));
		InverseKinematics::mat6x jacobianY = calcJacobian(targetBone, Kore::vec4(0, 1, 0, 0));
		InverseKinematics::mat6x jacobianZ = calcJacobian(targetBone, Kore::vec4(0, 0, 1, 0));
		
		// Get Pseude Inverse
		InverseKinematics::mat6x pseudoInvX = getPsevdoInverse(jacobianX);
		InverseKinematics::mat6x pseudoInvY = getPsevdoInverse(jacobianY);
		InverseKinematics::mat6x pseudoInvZ = getPsevdoInverse(jacobianZ);
		
		// Calculate the angles
		InverseKinematics::vec6 V;
		V[0] = diffPos.x(); V[1] = diffPos.y(); V[2] = diffPos.z();
		V[3] = diffRot.x(); V[4] = diffRot.y(); V[5] = diffRot.z();
		InverseKinematics::vec8 aThetaX = pseudoInvX * V;
		InverseKinematics::vec8 aThetaY = pseudoInvY * V;
		InverseKinematics::vec8 aThetaZ = pseudoInvZ * V;
		
		std::vector<float> theta;
		for (int i = 0; i < maxBones; ++i) {
			theta.push_back(aThetaX[i]);
			theta.push_back(aThetaY[i]);
			theta.push_back(aThetaZ[i]);
		}
		
		applyChanges(theta, targetBone);
		for (int i = 0; i < bones.size(); ++i) updateBonePosition(bones[i]);
	}
	return false;
}

InverseKinematics::mat6x InverseKinematics::calcJacobian(BoneNode* targetBone, Kore::vec4 rotAxis) {
	
	InverseKinematics::mat6x jacobian;
	
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

		jacobian[j][3] = oaj.x();
		jacobian[j][4] = oaj.y();
		jacobian[j][5] = oaj.z();
		
		targetBone = targetBone->parent;
		++j;
	}
	
	return jacobian;
}

InverseKinematics::mat6x InverseKinematics::getPsevdoInverse(InverseKinematics::mat6x jacobian) {
	InverseKinematics::mat6x inv;
	
	// Left pseudo inverse: (J^T * J ) ^-1 * J^T
	//InverseKinematics::mat6x inv = ((jacobian.Transpose() * jacobian).Invert() * jacobian.Transpose()).Transpose();
	
	// Right pseudo inverse : J^T * (J * J^T) ^−1
	//InverseKinematics::mat6x inv = (jacobian.Transpose() * (jacobian * jacobian.Transpose()).Invert()).Transpose();
	
	float lambda = 1; // Damping factor
	if (jacDim < maxBones) {
		// Left Damped pseudo-inverse
		InverseKinematics::mat6x6 id6 = InverseKinematics::mat6x6::Identity();
		inv = ((jacobian.Transpose() * jacobian  + id6 * lambda * lambda).Invert() * jacobian.Transpose()).Transpose();
	} else {
		// Right Damped pseudo-inverse
		InverseKinematics::mat8x8 id8 = InverseKinematics::mat8x8::Identity();
		inv = (jacobian.Transpose() * (jacobian * jacobian.Transpose() + id8 * lambda * lambda).Invert()).Transpose();
	}
	
	return inv;
}

void InverseKinematics::applyChanges(std::vector<float> theta, BoneNode* targetBone) {
	
	int i = 0;
	while (targetBone->nodeIndex != rootIndex) {
		BoneNode* bone = targetBone;
		
		float radX = theta[i];
		float radY = theta[i+1];
		float radZ = theta[i+2];
		/*radX = RotationUtility::getRadians(radX);
		radY = RotationUtility::getRadians(radY);
		radZ = RotationUtility::getRadians(radZ);*/
		
		//Kore::log(Kore::Info, "Bone %s -> x=%f y=%f z=%f", bone->boneName, radX, radY, radZ);
		
		// Interpolate between two quaternions if the angle is too big
		float delta = 50.f;
		if (radX > delta || radX < -delta || radY > delta || radY < -delta || radZ > delta || radZ < -delta)
			bone->interpolate = true;
		
		bone->rotation.x() += radX;
		bone->rotation.y() += radY;
		bone->rotation.z() += radZ;
		
		Kore::vec4 axis = bone->axes;
		if (axis.x() == 1) clampValue(bone->constrain[0].x(), bone->constrain[0].y(), &bone->rotation.x());
		if (axis.y() == 1) clampValue(bone->constrain[1].x(), bone->constrain[1].y(), &bone->rotation.y());
		if (axis.z() == 1) clampValue(bone->constrain[2].x(), bone->constrain[2].y(), &bone->rotation.z());
		
		Kore::Quaternion quat;
		RotationUtility::eulerToQuat(bone->rotation.x(), bone->rotation.y(), bone->rotation.z(), &quat);
		bone->quaternion = quat;
		
		// T * R * S
		quat.normalize();
		Kore::mat4 rotMat = quat.matrix().Transpose();
		bone->local = bone->transform * rotMat;
		//Kore::log(Info, "Bone %s -> angle: %f %f %f quaterion: %f %f %f", bone->boneName, bone->rotation.x(), bone->rotation.y(), bone->rotation.z(),  bone->desQuaternion.x, bone->desQuaternion.y, bone->desQuaternion.z);
		
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

void InverseKinematics::setJointConstraints() {
	BoneNode* nodeLeft;
	BoneNode* nodeRight;
	
	// clavicle
	/*nodeLeft = bones[7-1];
	nodeLeft->axes = Kore::vec4(0, 0, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-0.05, 0.05));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	
	nodeRight = bones[26-1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;*/
	
	//degrees × π / 180°
	
	// upperarm
	nodeLeft = bones[8-1];
	nodeLeft->axes = Kore::vec4(1, 1, 1, 0);
	nodeLeft->constrain.push_back(Kore::vec2(RotationUtility::getRadians(-90), RotationUtility::getRadians(150)));
	nodeLeft->constrain.push_back(Kore::vec2(RotationUtility::getRadians(-60), RotationUtility::getRadians(60)));
	nodeLeft->constrain.push_back(Kore::vec2(RotationUtility::getRadians(-20), RotationUtility::getRadians(60)));
	
	nodeRight = bones[27-1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// lowerarm
	nodeLeft = bones[9-1];
	nodeLeft->axes = Kore::vec4(1, 1, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(RotationUtility::getRadians(-30), RotationUtility::getRadians(90)));
	nodeLeft->constrain.push_back(Kore::vec2(RotationUtility::getRadians(-90), RotationUtility::getRadians(90)));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	
	nodeRight = bones[28-1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// thigh
	nodeLeft = bones[47-1];
	nodeLeft->axes = Kore::vec4(1, 0, 1, 0);
	nodeLeft->constrain.push_back(Kore::vec2(RotationUtility::getRadians(-90), RotationUtility::getRadians(60)));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(RotationUtility::getRadians(-80), RotationUtility::getRadians(80)));
	
	nodeRight = bones[51-1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
	// calf
	nodeLeft = bones[48-1];
	nodeLeft->axes = Kore::vec4(1, 0, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(RotationUtility::getRadians(0), RotationUtility::getRadians(150)));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	
	nodeRight = bones[52-1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;
	
}
