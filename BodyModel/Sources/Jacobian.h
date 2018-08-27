#include "MeshObject.h"
#include "BussIK/MatrixRmn.h"
#include "RotationUtility.h"
#include "Settings.h"

#include <Kore/Log.h>

struct BoneNode;

extern float dMaxPos[], dMaxRot[], lambda[];

template<int nJointDOFs = 6, bool posAndOrientation = true> class Jacobian {
	
public:
	std::vector<float> calcDeltaTheta(BoneNode* endEffektor, Kore::vec3 pos_soll, Kore::Quaternion rot_soll, int ikMode, float* dMax) {
		std::vector<float> deltaTheta;
		vec_n vec;
		Kore::vec3 p_aktuell = endEffektor->getPosition(); // Get current rotation and position of the end-effector
		vec_m deltaP = calcDeltaP(endEffektor, p_aktuell, pos_soll, rot_soll);
		mat_mxn jacobian = calcJacobian(endEffektor, p_aktuell);
		
		// set error
		errorPos = Kore::vec3(deltaP[0], deltaP[1], deltaP[2]).getLength();
		if (nDOFs == 6)
			errorRot = Kore::vec3(deltaP[3], deltaP[4], deltaP[5]).getLength();
		
		// clampMag
		if (dMax[ikMode] > nearNull) {
			Kore::vec3 clampedPos = clampMag(Kore::vec3(deltaP[0], deltaP[1], deltaP[2]), dMax[ikMode]);
			deltaP[0] = clampedPos[0];
			deltaP[1] = clampedPos[1];
			deltaP[2] = clampedPos[2];
		}
		
		switch (ikMode) {
			case 1:
				vec = calcDeltaThetaByPseudoInverse(jacobian, deltaP);
				break;
			case 2:
				vec = calcDeltaThetaByDLS(jacobian, deltaP);
				break;
			case 3:
				vec = calcDeltaThetaBySVD(jacobian, deltaP);
				break;
			case 4:
				vec = calcDeltaThetaByDLSwithSVD(jacobian, deltaP);
				break;
			case 5:
				vec = calcDeltaThetaBySDLS(jacobian, deltaP);
				break;
				
			default:
				vec = calcDeltaThetaByTranspose(jacobian, deltaP);
				break;
		}
		
		for (int n = 0; n < nJointDOFs; ++n)
			deltaTheta.push_back(vec[n]);
		
		return deltaTheta;
	}
	float getPositionError() {
		return errorPos;
	}
	float getRotationError() {
		return errorRot;
	}
	
private:
	typedef Kore::Matrix<nJointDOFs, posAndOrientation ? 6 : 3, float>                  mat_mxn;
	typedef Kore::Matrix<posAndOrientation ? 6 : 3, nJointDOFs, float>                  mat_nxm;
	typedef Kore::Matrix<posAndOrientation ? 6 : 3, posAndOrientation ? 6 : 3, float>   mat_mxm;
	typedef Kore::Matrix<nJointDOFs, nJointDOFs, float>                                 mat_nxn;
	typedef Kore::Vector<float, posAndOrientation ? 6 : 3>                              vec_m;
	typedef Kore::Vector<float, nJointDOFs>                                             vec_n;
	
	float   errorPos = -1.0f;
	float	errorRot = -1.0f;
	int     nDOFs = posAndOrientation ? 6 : 3;
	
	mat_mxm U;
	mat_nxn V;
	vec_m   d;
	
	vec_n calcDeltaThetaByTranspose(mat_mxn jacobian, vec_m deltaP) {
		mat_nxm jacobianTranspose = jacobian.Transpose();
		vec_n theta = jacobianTranspose * deltaP;
		
		return lambda[0] * theta;
	}
	vec_n calcDeltaThetaByPseudoInverse(mat_mxn jacobian, vec_m deltaP) {
		mat_nxm pseudoInverse = calcPseudoInverse(jacobian);
		vec_n theta = pseudoInverse * deltaP;
		
		return lambda[1] * theta;
	}
	vec_n calcDeltaThetaByDLS(mat_mxn jacobian, vec_m deltaP) {
		return calcPseudoInverse(jacobian, lambda[2]) * deltaP;
	}
	vec_n calcDeltaThetaBySVD(mat_mxn jacobian, vec_m deltaP) {
		calcSVD(jacobian);
		
		float max = MaxAbs(d);
		
		mat_nxm pseudoInverse;
		for (int i = 0; i < Min(nDOFs, nJointDOFs); ++i)
			if (fabs(d[i]) > lambda[3] * max) // modification to stabilize SVD
				for (int n = 0; n < nJointDOFs; ++n)
					for (int m = 0; m < nDOFs; ++m)
						pseudoInverse[n][m] += (1 / d[i]) * V[n][i] * U[m][i];
		
		return pseudoInverse * deltaP;
	}
	vec_n calcDeltaThetaByDLSwithSVD(mat_mxn jacobian, vec_m deltaP) {
		calcSVD(jacobian);
		
		mat_nxm dls;
		for (int i = 0; i < Min(nDOFs, nJointDOFs); ++i) {
			if (fabs(d[i]) > nearNull) {
				float l = d[i] / (Square(d[i]) + Square(lambda[4]));
				
				for (int n = 0; n < nJointDOFs; ++n)
					for (int m = 0; m < nDOFs; ++m)
						dls[n][m] += l * V[n][i] * U[m][i];
			}
		}
		
		return dls * deltaP;
	}
	vec_n calcDeltaThetaBySDLS(mat_mxn jacobian, vec_m deltaP) {
		calcSVD(jacobian);
		
		vec_n phi;
		for (int i = 0; i < Min(nDOFs, nJointDOFs); ++i) {
			vec_m u_i;
			vec_m s_i;
			float alpha_i = 0;
			for (int m = 0; m < nDOFs; ++m) {
				u_i[m] = U[m][i];
				s_i[m] = jacobian[m][i];
				alpha_i += u_i[m] * deltaP[m];
			}
			float N_i = u_i.getLength();
			
			if (
				fabs(d[i]) > nearNull &&
				N_i > nearNull &&
				fabs(alpha_i) > nearNull &&
				lambda[5] > nearNull
				) {
				float omegaInverse_i = 1.0 / d[i];
				
				vec_n v_i;
				float M_i = 0.0f;
				for (int n = 0; n < nJointDOFs; ++n) {
					v_i[n] = V[n][i];
					M_i += fabs(v_i[n]) * s_i.getLength();
				}
				M_i *= omegaInverse_i;
				
				float gamma_i = M_i > N_i ? N_i / M_i : 1;
				gamma_i *= lambda[5];
				
				phi += clampMaxAbs(omegaInverse_i * alpha_i * v_i, gamma_i);
			}
		}
		
		return clampMaxAbs(phi, lambda[5]);
	}
	
	// ---------------------------------------------------------
	
	vec_m calcDeltaP(BoneNode* endEffektor, Kore::vec3 p_aktuell, Kore::vec3 pos_soll, Kore::Quaternion rot_soll) {
		vec_m deltaP;
		
		// Calculate difference between desired position and actual position of the end effector
		Kore::vec3 deltaPos = pos_soll - p_aktuell;
		
		deltaP[0] = deltaPos.x();
		deltaP[1] = deltaPos.y();
		deltaP[2] = deltaPos.z();
		
		// Calculate difference between desired rotation and actual rotation
		if (nDOFs == 6) {
			Kore::Quaternion rot_aktuell;
			Kore::RotationUtility::getOrientation(&endEffektor->combined, &rot_aktuell);
			
			Kore::Quaternion rot_soll_temp = rot_soll;
			rot_soll_temp.normalize();
			
			Kore::Quaternion deltaRot_quat = rot_soll_temp.rotated(rot_aktuell.invert());
			if (deltaRot_quat.w < 0) deltaRot_quat = deltaRot_quat.scaled(-1);
			
			Kore::vec3 deltaRot = Kore::vec3(0, 0, 0);
			Kore::RotationUtility::quatToEuler(&deltaRot_quat, &deltaRot.x(), &deltaRot.y(), &deltaRot.z());
			
			deltaP[3] = deltaRot.x();
			deltaP[4] = deltaRot.y();
			deltaP[5] = deltaRot.z();
		}
		
		return deltaP;
	}
	
	mat_mxn calcJacobian(BoneNode* endEffektor, Kore::vec3 p_aktuell) {
		Jacobian::mat_mxn jacobianMatrix;
		BoneNode* bone = endEffektor;
		
		int joint = 0;
		while (bone->initialized && joint < nJointDOFs) {
			Kore::vec3 axes = bone->axes;
			
			if (axes.x() == 1.0 && joint < nJointDOFs) {
				vec_m column = calcJacobianColumn(bone, p_aktuell, Kore::vec3(1, 0, 0));
				for (int i = 0; i < nDOFs; ++i) jacobianMatrix[i][joint] = column[i];
				joint += 1;
			}
			if (axes.y() == 1.0 && joint < nJointDOFs) {
				vec_m column = calcJacobianColumn(bone, p_aktuell, Kore::vec3(0, 1, 0));
				for (int i = 0; i < nDOFs; ++i) jacobianMatrix[i][joint] = column[i];
				joint += 1;
			}
			if (axes.z() == 1.0 && joint < nJointDOFs) {
				vec_m column = calcJacobianColumn(bone, p_aktuell, Kore::vec3(0, 0, 1));
				for (int i = 0; i < nDOFs; ++i) jacobianMatrix[i][joint] = column[i];
				joint += 1;
			}
			
			bone = bone->parent;
		}
		
		return jacobianMatrix;
	}
	
	vec_m calcJacobianColumn(BoneNode* bone, Kore::vec3 p_aktuell, Kore::vec3 rotAxis) {
		vec_m column;
		
		// Get rotation and position vector of the current bone
		Kore::vec3 p_j = bone->getPosition();
		
		// get rotation-axis
		Kore::vec4 v_j = bone->combined * Kore::vec4(rotAxis.x(), rotAxis.y(), rotAxis.z(), 0);
		
		// cross-product
		Kore::vec3 pTheta = Kore::vec3(v_j.x(), v_j.y(), v_j.z()).cross(p_aktuell - p_j);
		
		if (nDOFs > 0) column[0] = pTheta.x();
		if (nDOFs > 1) column[1] = pTheta.y();
		if (nDOFs > 2) column[2] = pTheta.z();
		if (nDOFs > 3) column[3] = v_j.x();
		if (nDOFs > 4) column[4] = v_j.y();
		if (nDOFs > 5) column[5] = v_j.z();
		
		return column;
	}
	
	mat_nxm calcPseudoInverse(mat_mxn jacobian, float l = 0) { // lambda != 0 => DLS!
		mat_nxm pseudoInverse;
		mat_nxm transpose = jacobian.Transpose();
		
		if (nDOFs <= nJointDOFs) { // m <= n
			// Left Damped pseudo-inverse
			Jacobian::mat_nxn id;
			if (l > nearNull) id = Jacobian::mat_nxn::Identity() * Square(l);
			
			pseudoInverse = (transpose * jacobian + id).Invert() * transpose;
		} else {
			// Right Damped pseudo-inverse
			Jacobian::mat_mxm id;
			if (l > nearNull) id = Jacobian::mat_mxm::Identity() * Square(l);
			
			pseudoInverse = transpose * (jacobian * transpose + id).Invert();
		}
		
		return pseudoInverse;
	}
	
	void calcSVD(Jacobian::mat_mxn jacobian) {
		MatrixRmn J = MatrixRmn(nDOFs, nJointDOFs);
		MatrixRmn U = MatrixRmn(nDOFs, nDOFs);
		MatrixRmn V = MatrixRmn(nJointDOFs, nJointDOFs);
		VectorRn d = VectorRn(Min(nDOFs, nJointDOFs));
		
		for (int m = 0; m < nDOFs; ++m)
			for (int n = 0; n < nJointDOFs; ++n)
				J.Set(m, n, (double) jacobian[m][n]);
		
		J.ComputeSVD(U, d, V);
		assert(J.DebugCheckSVD(U, d , V));
		
		for (int m = 0; m < Max(nDOFs, nJointDOFs); ++m) {
			for (int n = 0; n < Max(nDOFs, nJointDOFs); ++n) {
				if (m < nDOFs && n < nDOFs)
					Jacobian::U[m][n] = (float) U.Get(m, n);
				
				if (m < nJointDOFs && n < nJointDOFs)
					Jacobian::V[m][n] = (float) V.Get(m, n);
				
				if (m == n && m < Min(nDOFs, nJointDOFs))
					Jacobian::d[m] = (float) d.Get(m);
			}
		}
	}
	
	Kore::vec3 clampMag(Kore::vec3 vec, float gamma_i) {
		float length = vec.getLength();
		
		if (length > gamma_i)
			for (int n = 0; n < 3; ++n)
				vec[n] = gamma_i * vec[n] / length;
		
		return vec;
	}
	
	float MaxAbs(vec_m vec) {
		float max = 0;
		
		for (int m = 0; m < nDOFs; ++m) {
			float val = fabs(vec[m]);
			
			if (val > max) max = val;
		}
		
		return max;
	}
	
	vec_n clampMaxAbs(vec_n vec, float gamma_i) {
		float max = 0;
		
		for (int n = 0; n < nJointDOFs; ++n) {
			float val = fabs(vec[n]);
			
			if (val > max) max = val;
		}
		
		if (max > gamma_i)
			for (int n = 0; n < nJointDOFs; ++n)
				vec[n] = gamma_i * vec[n] / max;
		
		return vec;
	}
};
