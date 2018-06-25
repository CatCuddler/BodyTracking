//
//  Jacobian.cpp
//  BodyTracking
//
//  Created by Philipp Achenbach on 16.05.18.
//  Copyright © 2018 KTX Software Development. All rights reserved.
//

#include "pch.h"
#include "Jacobian.h"

#include "RotationUtility.h"
#include "MeshObject.h"

#include "Jacobian/MatrixRmn.h"

#include <Kore/Log.h>

Jacobian::Jacobian(BoneNode* targetBone, Kore::vec4 pos, Kore::Quaternion rot, int n, bool posAndOrientation) {
    endEffektor = targetBone;
    pos_soll = pos;
    rot_soll = rot;
    nJointDOFs = n < maxJointDOFs ? n : maxJointDOFs;
    nDOFs = posAndOrientation ? 6 : 3;
}

float Jacobian::getError() {
    return error || calcDeltaP().getLength();
}

std::vector<float> Jacobian::calcDeltaTheta(int ikMode) {
    std::vector<float> deltaTheta;
    vec_n vec;
    
    switch (ikMode) {
        case 1:
            vec = calcDeltaThetaByPseudoInverse();
            break;
        case 2:
            vec = calcDeltaThetaByDLS();
            break;
        case 3:
            vec = calcDeltaThetaBySVD();
            break;
        case 4:
            vec = calcDeltaThetaByDLSwithSVD();
            break;
        case 5:
            vec = calcDeltaThetaBySDLS();
            break;
            
        default:
            vec = calcDeltaThetaByTranspose();
            break;
    }
    
    for (int n = 0; n < nJointDOFs; ++n)
        deltaTheta.push_back(vec[n]);
    
    return deltaTheta;
}

Jacobian::vec_n Jacobian::calcDeltaThetaByTranspose() {
    return calcJacobian().Transpose() * calcDeltaP();
}

Jacobian::vec_n Jacobian::calcDeltaThetaByPseudoInverse() {
    return calcPseudoInverse(lambdaPseudoInverse) * calcDeltaP();
}

Jacobian::vec_n Jacobian::calcDeltaThetaByDLS() {
    return calcPseudoInverse(lambdaDLS) * calcDeltaP();
}

Jacobian::vec_n Jacobian::calcDeltaThetaBySVD() {
    calcSVD(calcJacobian());
    
    mat_nxm D;
    for (int i = 0; i < Min(nDOFs, nJointDOFs); ++i)
        D[i][i] = fabs(d[i]) > (lambdaSVD * MaxAbs(d)) ? (1 / d[i]) : 0;
    
    return V * D * U.Transpose() * calcDeltaP();
}

Jacobian::vec_n Jacobian::calcDeltaThetaByDLSwithSVD() {
    calcSVD(calcJacobian());
    
    mat_nxm E;
    for (int i = 0; i < Min(nDOFs, nJointDOFs); ++i)
        E[i][i] = d[i] / (Square(d[i]) + Square(lambdaDLSwithSVD));
    
    return V * E * U.Transpose() * calcDeltaP();
}

Jacobian::vec_n Jacobian::calcDeltaThetaBySDLS() {
    Jacobian::mat_mxn jacobian = calcJacobian();
    vec_m deltaP = calcDeltaP();
    calcSVD(jacobian);
    
    vec_n theta;
    for (int i = 0; i < Min(nDOFs, nJointDOFs); ++i) {
        vec_m u_i;
        for (int j = 0; j < nDOFs; ++j) {
            u_i[j] = U[j][i];
        }
        
        vec_n v_i;
        for (int j = 0; j < nJointDOFs; ++j)
            v_i[j] = V[j][i];
        
        // alpha_i = uT_i * deltaP
        float alpha_i = 0.0;
        for (int m = 0; m < nDOFs; ++m)
            alpha_i += u_i[m] * deltaP[m];
        
        float omegaInverse_i = 1.0 / d[i];
        
        float M_i = 0.0;
        for (int l = 0; l < (nDOFs / 3); ++l)
            for (int j = 0; j < nJointDOFs; ++j)
                M_i += fabs(V[j][i]) * fabs(jacobian[l][j]);
        M_i *= omegaInverse_i;
        
        float N_i = 1.0; // u_i.getLength();
        float gamma_i = Min(1, N_i / M_i) * lambdaSDLS; // todo: Kette durchgehen und maximalen Winkel ermitteln statt lambdaSDLS!
        
        theta += clampMaxAbs(omegaInverse_i * alpha_i * v_i, gamma_i);
    }
    
    return theta;
}

// ################################################################

Jacobian::vec_m Jacobian::calcDeltaP() {
    vec_m deltaP;
    
    // Calculate difference between desired position and actual position of the end effector
    Kore::vec3 deltaPos = pos_soll - calcPosition(endEffektor);
    if (nDOFs > 0) deltaP[0] = deltaPos.x();
    if (nDOFs > 1) deltaP[1] = deltaPos.y();
    if (nDOFs > 2) deltaP[2] = deltaPos.z();
    
    // Calculate difference between desired rotation and actual rotation
    if (nDOFs > 3) {
        Kore::Quaternion rot_aktuell;
        Kore::RotationUtility::getOrientation(&endEffektor->combined, &rot_aktuell);
        
        Kore::Quaternion rot_soll_temp = rot_soll;
        rot_soll_temp.normalize();
        
        Kore::Quaternion deltaRot_quat = rot_soll_temp.rotated(rot_aktuell.invert());
        if (deltaRot_quat.w < 0) deltaRot_quat = deltaRot_quat.scaled(-1);
        
        Kore::vec3 deltaRot = Kore::vec3(0, 0, 0);
        Kore::RotationUtility::quatToEuler(&deltaRot_quat, &deltaRot.x(), &deltaRot.y(), &deltaRot.z());
        
        deltaP[3] = deltaRot.x();
        if (nDOFs > 4) deltaP[4] = deltaRot.y();
        if (nDOFs > 5) deltaP[5] = deltaRot.z();
    }
    
    // set error
    error = deltaP.getLength();
    
    return deltaP;
}

Jacobian::mat_mxn Jacobian::calcJacobian() {
    Jacobian::mat_mxn jacobianMatrix;
    BoneNode* bone = endEffektor;
    
    // Get current rotation and position of the end-effector
    Kore::vec3 p_aktuell = calcPosition(bone);
    
    int joint = 0;
    while (bone->initialized) {
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

Jacobian::vec_m Jacobian::calcJacobianColumn(BoneNode* bone, Kore::vec3 p_aktuell, Kore::vec3 rotAxis) {
    vec_m column;
    
    // Get rotation and position vector of the current bone
    Kore::vec3 p_j = calcPosition(bone);
    
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

Jacobian::mat_nxm Jacobian::calcPseudoInverse(float lambda) { // lambda != 0 => DLS!
    mat_mxn jacobian = calcJacobian();
    
    if (nDOFs <= nJointDOFs) { // m <= n
        // Left Damped pseudo-inverse
        return (jacobian.Transpose() * jacobian + Jacobian::mat_nxn::Identity() * Square(lambda)).Invert() * jacobian.Transpose();
    } else {
        // Right Damped pseudo-inverse
        return jacobian.Transpose() * (jacobian * jacobian.Transpose() + Jacobian::mat_mxm::Identity() * Square(lambda)).Invert();
    }
}

Kore::vec3 Jacobian::calcPosition(BoneNode* bone) {
    Kore::vec3 result;
    
    // from quat to euler!
    Kore::vec4 quat = bone->combined * Kore::vec4(0, 0, 0, 1);
    quat *= 1.0 / quat.w();
    
    result.x() = quat.x();
    result.y() = quat.y();
    result.z() = quat.z();
    
    return result;
}

void Jacobian::calcSVD(Jacobian::mat_mxn jacobian) {
    MatrixRmn J = MatrixRmn(nDOFs, nJointDOFs);
    MatrixRmn U = MatrixRmn(nDOFs, nDOFs);
    MatrixRmn V = MatrixRmn(nJointDOFs, nJointDOFs);
    VectorRn d = VectorRn(Min(nDOFs, nJointDOFs));
    
    for (int m = 0; m < nDOFs; ++m) {
        for (int n = 0; n < nJointDOFs; ++n) {
            J.Set(m, n, (double) jacobian[m][n]);
        }
    }
    
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

Jacobian::vec_n Jacobian::clampMaxAbs(vec_n vec, float gamma_i) {
    float gammaAbs_i = fabs(gamma_i);
    float maxValue = MaxAbs(vec, gammaAbs_i);
    
    // scale vector to gamma_i as max
    if (maxValue != gammaAbs_i)
        for (int n = 0; n < nJointDOFs; ++n)
            vec[n] = vec[n] / maxValue * gammaAbs_i;
    
    return vec;
}

float Jacobian::MaxAbs(vec_m vec) {
    float result = 0.0;
    
    for (int m = 0; m < nDOFs; ++m)
        result = Max(fabs(vec[m]), result);
    
    return result;
}

float Jacobian::MaxAbs(vec_n vec, float start) {
    float result = start;
    
    for (int n = 0; n < nJointDOFs; ++n)
        result = Max(fabs(vec[n]), result);
    
    return result;
}
