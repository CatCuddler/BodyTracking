#include "pch.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

#include <Kore/Log.h>

#include <vector>

InverseKinematics::InverseKinematics() : maxSteps(10), maxError(0.1f) {
}

bool InverseKinematics::inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone) {
	
	if (!targetBone->initialized) return false;
	// TODO: calculate parent count
	
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
		InverseKinematics::mat54x pseudoInv = getPsevdoInverse(jacobian);
		//Kore::vec6 theta = pseudoInv * Kore::vec6(dif.x(), dif.y(), dif.z(), 0, 0, 0);
		
		MatrixX V(6, 1);
		V[0][0] = dif.x();
		V[1][0] = dif.y();
		V[2][0] = dif.z();
		V[3][0] = 0;
		V[4][0] = 0;
		V[5][0] = 0;
		
		MatrixX P(54, 6);
		for (int i = 0; i < 54; ++i) {
			for (int j = 0; j < 6; ++j) {
				P[i][j] = pseudoInv.get(j, i);
			}
		}
		
		MatrixX theta;
		theta.Mult(P, V);
		
		//targetBone->local = targetBone->local.Translation(delta.x(), delta.y(), delta.z());
		//targetBone->local = targetBone->local.Rotation(theta.x(), theta.y(), theta.z());
		
		applyChanges(theta, targetBone);
	}
	return false;
}

InverseKinematics::mat6x InverseKinematics::calcJacobian(BoneNode* targetBone) {
	
	std::vector<BoneNode*> parentNodes;
	BoneNode* bone = targetBone;
	while (bone->nodeIndex != 3) {
		parentNodes.push_back(bone);
		bone = bone->parent;
	}
	
	InverseKinematics::mat6x jacobian;
	
	Kore::vec4 orn = targetBone->combined * Kore::vec4(0, 0, 0, 1);
	
	// Add end effector
	/*Kore::vec4 zi = vec4(0, 0, 1, 0);
	Kore::vec4 ri = vec4(0, 0, 0, 1);
	vec3 cross = zi.cross(orn - ri);
	
	int i = 0;
	jacobian[i][0] = cross.x();
	jacobian[i][1] = cross.y();
	jacobian[i][2] = cross.z();
	
	jacobian[i][3] = zi.x();
	jacobian[i][4] = zi.y();
	jacobian[i][5] = zi.z();
	++i;*/
	
	int i = 0;
	//while (targetBone->nodeIndex != 1) {
	for (std::vector<BoneNode*>::reverse_iterator it = parentNodes.rbegin() ; it != parentNodes.rend(); ++it) {
		BoneNode* bone = *it;
		
		if (bone->nodeIndex == targetBone->nodeIndex) break;
		
		//Kore::log(Info, "Bone %s", bone->boneName);
		
		Kore::vec4 zi = bone->combined * Kore::vec4(0, 0, 1, 0);
		Kore::vec4 ri = bone->combined * Kore::vec4(0, 0, 0, 1);
		Kore::vec4 cross = zi.cross(orn - ri);
		
		jacobian[i][0] = cross.x();
		jacobian[i][1] = cross.y();
		jacobian[i][2] = cross.z();
		
		jacobian[i][3] = zi.x();
		jacobian[i][4] = zi.y();
		jacobian[i][5] = zi.z();
		
	//	targetBone = targetBone->parent;
		++i;
	}
	
	return jacobian;
}

InverseKinematics::mat54x InverseKinematics::getPsevdoInverse(InverseKinematics::mat6x jacobian) {
	
	// Left pseudo inverse: (J^T * J ) ^-1 * J^T
	//InverseKinematics::mat54x inv = (jacobian.Transpose() * jacobian).Invert() * jacobian.Transpose();
	
	// Right pseudo inverse : J^T * (J * J^T) ^−1
	InverseKinematics::mat54x inv = jacobian.Transpose() * (jacobian * jacobian.Transpose()).Invert();
	
	return inv;
}

void InverseKinematics::applyChanges(MatrixX theta, BoneNode* targetBone) {
	std::vector<BoneNode*> parentNodes;
	BoneNode* bone = targetBone;
	while (bone->nodeIndex != 3) {
		parentNodes.push_back(bone);
		bone = bone->parent;
	}
	
	int i = 0;
	//while (targetBone->nodeIndex != 1) {
	for (std::vector<BoneNode*>::reverse_iterator it = parentNodes.rbegin() ; it != parentNodes.rend(); ++it) {
		BoneNode* bone = *it;
		//Kore::vec4 angle = targetBone->combined * Kore::vec4(0, 0, 1, 0);
		
		float angle = theta[i][0];
		
		Kore::log(Info, "Bone %s -> angle %f", bone->boneName, angle);
		bone->local *= bone->local.RotationZ(angle);
		
		++i;
	}
	
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
