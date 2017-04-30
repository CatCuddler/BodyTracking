#include "pch.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

#include <Kore/Log.h>

InverseKinematics::InverseKinematics() : boneCount(0), maxSteps(10), maxError(0.1f) {
}

bool InverseKinematics::inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone) {
	
	if (!targetBone->initialized) return false;
	// TODO: calculate parent count
	
	boneCount = 0;
	BoneNode* endEffector = targetBone;
	while (endEffector->nodeIndex != 1) {
		Kore::log(Info, "node %s", endEffector->boneName);

		endEffector = endEffector->parent;
		++boneCount;
	}
	
	for (int i = 0; i < maxSteps; ++i) {
		
		// Calculate error between desired position and actual position of the end effector
		Kore::vec4 currentPos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
		Kore::vec4 dif = desiredPos - currentPos;
		
		float err = dif.getLength();
		if (err < maxError) {
			return true;
		}
		
		// Calculate Jacobi Matrix
		InverseKinematics::mat6x jacobian = calcJacobian(targetBone);
		
		// Get pseude inverse
		InverseKinematics::mat6x pseudoInv = getPsevdoInverse(jacobian);
		//Kore::vec4 delta = pseudoInv * dif;
		
		//targetBone->local = targetBone->local.Translation(delta.x(), delta.y(), delta.z());
		//targetBone->local = targetBone->local.Rotation(delta.x(), delta.y(), delta.z());
	}
	return false;
}

InverseKinematics::mat6x InverseKinematics::calcJacobian(BoneNode* targetBone) {
	
	Kore::vec4 endEffector(0, -0.78784620240976644, 0.13891854213354426, 1);
	Kore::vec4 posBone1(0, 0, 0, 0);
	
	InverseKinematics::mat6x jacobian;
	
	Kore::vec4 z0(0, 0, 1, 0);
	Kore::vec4 orn = endEffector;//targetBone->combined * Kore::vec4(0, 0, 0, 1);
	
	bool isEndEffector = true;

	int i = 0;
	while (targetBone->nodeIndex != 1) {
		Kore::log(Info, "node %s", targetBone->boneName);
		
		Kore::vec4 ri = endEffector;//targetBone->combined * Kore::vec4(0, 0, 0, 1);
		Kore::vec4 zi = targetBone->combined * Kore::vec4(-1, 0, 0, 0);
		if (isEndEffector) {
			//zi = vec4(0, 0, 1, 0);
			zi = vec4(-1, 0, 0, 0);
			ri = vec4(0, -1.3878462024097664, 0.13891854213354426, 1);//vec4(1, 1, 1, 1);
			isEndEffector = false;
		}
		vec3 cross = zi.cross(orn - ri);
		
		jacobian[0][i] = cross.x();
		jacobian[1][i] = cross.y();
		jacobian[2][i] = cross.z();
		
		jacobian[3][i] = zi.x();
		jacobian[4][i] = zi.y();
		jacobian[5][i] = zi.z();
		
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
