#include "pch.h"
#include "InverseKinematics.h"
#include "MeshObject.h"

InverseKinematics::InverseKinematics() : maxSteps(10), maxError(0.1f) {

}

bool InverseKinematics::inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone) {
	
	// TODO: calculate parent count
	
	for (int i = 0; i < maxSteps; ++i) {
		
		// Calculate error between desired position and actual position of the end effector
		Kore::vec4 pos = targetBone->combined * Kore::vec4(0, 0, 0, 1);
		Kore::vec6 cPos(pos.x(), pos.y(), pos.z(), 0, 0, 0);
		Kore::vec6 dPos(desiredPos.x(), desiredPos.y(), desiredPos.z(), 0, 0, 0);
		Kore::vec6 dif = dPos - cPos;
		
		float err = dif.normalize().getLength();
		if (err < maxError) {
			return true;
		}
		
		// Calculate Jacobi Matrix
		InverseKinematics::mat6x jacobian = calcJacobian(targetBone);
		
		// Get pseude inverse
		InverseKinematics::mat6x pseudoInv = getPsevdoInverse(jacobian);
		Kore::vec6 delta = pseudoInv * dif;
		
		targetBone->local = targetBone->local.Translation(delta.x(), delta.y(), delta.z());
	}
	return false;
}

InverseKinematics::mat6x InverseKinematics::calcJacobian(BoneNode* targetBone) {
	
	InverseKinematics::mat6x jacobian;
	
	Kore::vec3 z0(0.0f , 0.0f , 1.0f);
	Kore::vec3 zi;
	Kore::mat4 rotation = targetBone->combined;
	
	Kore::vec4 orn = targetBone->combined/*.Transpose()*/ * Kore::vec4(0, 0, 0, 1);
	int i = 0;
	while (targetBone->nodeIndex != 1) {
		Kore::vec4 ri = targetBone->combined/*.Transpose()*/ * Kore::vec4(0, 0, 0, 1);
		Kore::vec4 zi = targetBone->combined * Kore::vec4(0, 0, 1, 0);
		vec3 cross = zi.cross(orn - ri);
		
		jacobian[0][i] = cross.x();
		jacobian[1][i] = cross.y();
		jacobian[2][i] = cross.x();
		
		jacobian[3][i] = zi.x();
		jacobian[4][i] = zi.y();
		jacobian[5][i] = zi.x();
		
		targetBone = targetBone->parent;
		++i;
	}
	
	return jacobian;
}

InverseKinematics::mat6x InverseKinematics::getPsevdoInverse(InverseKinematics::mat6x jacobian) {
	// Left pseudo inverse: (J^T * J ) ^-1 * J^T
	InverseKinematics::mat6x inv = ((jacobian.Transpose() * jacobian).Invert() * jacobian.Transpose()).Transpose();
	
	// Right pseudo inverse : J^T * (J * J^T) ^−1
	//InverseKinematics::matrix6 inv = (jacobian.Transpose() * (jacobian * jacobian.Transpose()).Invert()).Transpose();
	
	return inv;
}
