#include "pch.h"
#include "InverseKinematics.h"

#include "RotationUtility.h"
#include "MeshObject.h"

#include <Kore/Log.h>

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec, int maxSteps) : maxSteps(maxSteps), maxError(0.01f), rootIndex(2), clamp(false), sumIter(0), totalNum(0) {
	bones = boneVec;
	setJointConstraints();
}

bool InverseKinematics::inverseKinematics(BoneNode* targetBone, Kore::vec4 desiredPosition, Kore::Quaternion desiredRotation, bool posAndRot) {

	if (!targetBone->initialized) return false;

	//Kore::log(Kore::Info, "desPos %f %f %f", desiredPosition.x(), desiredPosition.y(), desiredPosition.z());
	//Kore::log(Kore::Info, "desRot %f %f %f %f", desiredRotation.w, desiredRotation.x, desiredRotation.y, desiredRotation.z);

	for (int i = 0; i < maxSteps; ++i) {
		//log(Info, "Iteration %i", i);

		// Calculate error between desired position and actual position of the end effector
		Kore::vec4 currentPosition = targetBone->combined * Kore::vec4(0, 0, 0, 1);
		currentPosition *= 1.0 / currentPosition.w();
		Kore::vec4 diffPos = desiredPosition - currentPosition;

		Kore::vec3 diffRot = Kore::vec3(0, 0, 0);
		if (posAndRot) {
			// Calculate error between deisred rotation and actual rotation
			Kore::Quaternion curQuat;
			Kore::RotationUtility::getOrientation(&targetBone->combined, &curQuat);
			Kore::Quaternion desQuat = desiredRotation;
			desQuat.normalize();

			//Kore::mat4 rotErr = desQuat.matrix().Transpose() * curQuat.matrix().Transpose().Invert();
			//Kore::Quaternion quatDiff;
			//RotationUtility::getOrientation(&rotErr, &quatDiff);

			Kore::Quaternion quatDiff = desQuat.rotated(curQuat.invert());
			if (quatDiff.w < 0) quatDiff = quatDiff.scaled(-1);

			Kore::RotationUtility::quatToEuler(&quatDiff, &diffRot.x(), &diffRot.y(), &diffRot.z());

			// Dont enforce joint limits by clamping if we know the desired rotation
			//clamp = false;
		}
		else {
			// Force joint limits, if we do not know the desired rotation
			//clamp = true;
		}

		InverseKinematics::vec6 V;
		V[0] = diffPos.x(); V[1] = diffPos.y(); V[2] = diffPos.z();
		V[3] = diffRot.x(); V[4] = diffRot.y(); V[5] = diffRot.z();

		float error = V.getLength();
		//log(Info, "error %f \t diffPos %f \t error diffRot %f", error, diffPos.getLength(), diffRot.getLength());
		if (error < maxError) {
			sumIter += i;
			++totalNum;

			/*Kore::log(Kore::Info, "Inverse kinematics terminated after %i iterations.", i);
			Kore::log(Kore::Info, "Position error: %f", diffPos.getLength());
			Kore::log(Kore::Info, "Attitude error: %f", diffRot.getLength());*/
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
		Kore::vec3 aThetaX = pseudoInvX * V;
		Kore::vec3 aThetaY = pseudoInvY * V;
		Kore::vec3 aThetaZ = pseudoInvZ * V;

		std::vector<float> theta;
		for (int i = 0; i < maxBones; ++i) {
			theta.push_back(aThetaX[i]);
			theta.push_back(aThetaY[i]);
			theta.push_back(aThetaZ[i]);
		}

		applyChanges(theta, targetBone);
		applyJointConstraints(targetBone);
		for (int i = 0; i < bones.size(); ++i) updateBonePosition(bones[i]);
	}

	sumIter += maxSteps;
	++totalNum;

	return false;
}

InverseKinematics::mat6x InverseKinematics::calcJacobian(BoneNode* targetBone, Kore::vec4 rotAxis) {

	InverseKinematics::mat6x jacobian;

	// Get current position of the end-effector
	Kore::vec4 opn = targetBone->combined * Kore::vec4(0, 0, 0, 1);
	opn *= 1.0 / opn.w();

	for (int b = 0; b < maxBones; ++b) {
		BoneNode* bone = targetBone;

		// Get rotation and position vector of the current bone
		Kore::vec4 oaj = Kore::vec4(0, 0, 0, 0);
		Kore::vec4 opj = bone->combined * Kore::vec4(0, 0, 0, 1);
		opj *= 1.0 / opj.w();

		Kore::vec3 axes = bone->axes;
		if ((axes.x() == 1 && axes.x() == rotAxis.x()) || (axes.y() == 1 && axes.y() == rotAxis.y()) || (axes.z() == 1 && axes.z() == rotAxis.z())) {
			oaj = bone->combined * rotAxis;
		}

		// Calculate cross product
		Kore::vec4 cross = oaj.cross(opn - opj);

		jacobian[b][0] = cross.x();
		jacobian[b][1] = cross.y();
		jacobian[b][2] = cross.z();

		jacobian[b][3] = oaj.x();
		jacobian[b][4] = oaj.y();
		jacobian[b][5] = oaj.z();

		targetBone = targetBone->parent;
	}

	return jacobian;
}

InverseKinematics::mat6x InverseKinematics::getPsevdoInverse(InverseKinematics::mat6x jacobian) {
	InverseKinematics::mat6x inv;

	float lambda = 1;//0.001; // Damping factor
	if (jacDim < maxBones) {
		// Left Damped pseudo-inverse
		InverseKinematics::mat6x6 id6 = InverseKinematics::mat6x6::Identity();
		inv = ((jacobian.Transpose() * jacobian + id6 * lambda * lambda).Invert() * jacobian.Transpose()).Transpose();
	}
	else {
		// Right Damped pseudo-inverse
		Kore::mat3x3 id3 = Kore::mat3x3::Identity();
		inv = (jacobian.Transpose() * (jacobian * jacobian.Transpose() + id3 * lambda * lambda).Invert()).Transpose();
	}

	return inv;
}

void InverseKinematics::applyChanges(std::vector<float> theta, BoneNode* targetBone) {

	int i = 0;
	for (int b = 0; b < maxBones; ++b) {
		BoneNode* bone = targetBone;

		float radX = theta[i];
		float radY = theta[i + 1];
		float radZ = theta[i + 2];

		//Kore::log(Kore::Info, "Bone %s -> x=%f y=%f z=%f", bone->boneName, radX, radY, radZ);

		bone->quaternion.rotate(Kore::Quaternion(Kore::vec3(1, 0, 0), radX));
		bone->quaternion.rotate(Kore::Quaternion(Kore::vec3(0, 1, 0), radY));
		bone->quaternion.rotate(Kore::Quaternion(Kore::vec3(0, 0, 1), radZ));
		bone->quaternion.normalize();

		//log(Info, "%s %f %f %f %f", bone->boneName, bone->quaternion.w, bone->quaternion.x, bone->quaternion.y, bone->quaternion.z);

		Kore::mat4 rotMat = bone->quaternion.matrix().Transpose();
		bone->local = bone->transform * rotMat;

		targetBone = targetBone->parent;
		i = i + 3;
	}

}

void InverseKinematics::applyJointConstraints(BoneNode* targetBone) {
	for (int b = 0; b < maxBones; ++b) {
		BoneNode* bone = targetBone;

		clamp = true;
		if (clamp) {
			Kore::vec3 rot;
			Kore::RotationUtility::quatToEuler(&bone->quaternion, &rot.x(), &rot.y(), &rot.z());

			if (bone->axes.x() == 1) clampValue(bone->constrain[0].x(), bone->constrain[0].y(), &rot.x());
			if (bone->axes.y() == 1) clampValue(bone->constrain[1].x(), bone->constrain[1].y(), &rot.y());
			if (bone->axes.z() == 1) clampValue(bone->constrain[2].x(), bone->constrain[2].y(), &rot.z());

			Kore::RotationUtility::eulerToQuat(rot.x(), rot.y(), rot.z(), &bone->quaternion);

			bone->quaternion.normalize();

			Kore::mat4 rotMat = bone->quaternion.matrix().Transpose();
			bone->local = bone->transform * rotMat;
		}
		targetBone = targetBone->parent;
	}
}

bool InverseKinematics::clampValue(float minVal, float maxVal, float* value) {
	if (minVal > maxVal) {
		float temp = minVal;
		minVal = maxVal;
		maxVal = temp;
	}

	if (*value < minVal) {
		*value = minVal;
		return true;
	}
	else if (*value > maxVal) {
		*value = maxVal;
		return true;
	}
	return false;
}

void InverseKinematics::updateBonePosition(BoneNode* bone) {
	bone->combined = bone->parent->combined * bone->local;
}

void InverseKinematics::setJointConstraints() {
	BoneNode* nodeLeft;
	BoneNode* nodeRight;

	// pelvis
	nodeLeft = bones[3 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));

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
	nodeLeft = bones[8 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 2.0f, 2.0f * Kore::pi / 3.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 2.0f, Kore::pi / 3.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 6.0f, 2.0f * Kore::pi / 3.0f));

	nodeRight = bones[27 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);

	// lowerarm
	nodeLeft = bones[9 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));//-Kore::pi / 6.0f, 2.0f * Kore::pi / 3.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));//-Kore::pi / 3.0f, Kore::pi / 6.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));//-Kore::pi / 8.0f, Kore::pi / 8.0f));

	nodeRight = bones[28 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);


	// thigh
	nodeLeft = bones[47 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-5.0 * Kore::pi / 6.0f, Kore::pi / 3.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 8.0f, Kore::pi / 8.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 2.0f, Kore::pi / 2.0f));

	nodeRight = bones[51 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);

	// calf
	nodeLeft = bones[48 - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(0, 5.0 * Kore::pi / 6.0f));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));

	nodeRight = bones[52 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);

	// foot
	nodeLeft = bones[49 - 1];
	nodeLeft->axes = Kore::vec3(0, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));

	nodeRight = bones[53 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);
}

float InverseKinematics::getAverageIter() {
	float average = sumIter / (float)totalNum;
	return average;
}