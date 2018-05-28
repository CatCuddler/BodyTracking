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

#include <Kore/Log.h>

Jacobian::Jacobian(BoneNode* targetBone, Kore::vec4 pos, Kore::Quaternion rot) {
    endEffektor = targetBone;
    pos_soll = pos;
    rot_soll = rot;
    
    /* BoneNode* bone = targetBone;
    int counter = 0;
    while (bone->initialized) {
        Kore::vec3 axes = bone->axes;
        
        if (axes.x() == 1.0) counter += 1;
        if (axes.y() == 1.0) counter += 1;
        if (axes.z() == 1.0) counter += 1;
        
        bone = bone->parent;
    }
    log(Kore::Info, "Die Anzahl der Gelenke-Freiheitsgrade ist %i", counter); */
}

float Jacobian::getError() {
    return (!deltaP.isZero() ? deltaP : calcDeltaP()).getLength();
}

Jacobian::vec_n Jacobian::getDeltaThetaByTranspose() {
    // Get Jacobian
    mat_mxn jacobian = calcJacobian();
    
    // Get deltaP
    deltaP = calcDeltaP();
    
    // Calculate the angles
    return jacobian.Transpose() * deltaP;
}

Jacobian::vec_n Jacobian::getDeltaThetaByPseudoInverse() {
    return getDeltaThetaByDLS(0);
}

Jacobian::vec_n Jacobian::getDeltaThetaByDLS(float lambda) {
    // Get Jacobian
    mat_mxn jacobian = calcJacobian();
    
    // Get Pseudo Inverse
    mat_nxm pseudoInv = calcPseudoInverse(jacobian, lambda);
     
    // Get deltaP
    deltaP = calcDeltaP();
    
    // Calculate the angles
    return pseudoInv * deltaP;
}

Jacobian::vec_m Jacobian::calcDeltaP() {
    vec_m deltaP;
    
    // Calculate difference between desired position and actual position of the end effector
    Kore::vec3 deltaPos = pos_soll - getPosition(endEffektor);
    if (nDOFs > 0) deltaP[0] = deltaPos.x();
    if (nDOFs > 1) deltaP[1] = deltaPos.y();
    if (nDOFs > 2) deltaP[2] = deltaPos.z();
    
    // Calculate difference between desired rotation and actual rotation
    if (nDOFs > 3) {
        Kore::vec3 deltaRot = Kore::vec3(0, 0, 0);
        Kore::Quaternion rot_aktuell;
        Kore::RotationUtility::getOrientation(&endEffektor->combined, &rot_aktuell);
        Kore::Quaternion desQuat = rot_soll;
        desQuat.normalize();
        Kore::Quaternion quatDiff = desQuat.rotated(rot_aktuell.invert());
        if (quatDiff.w < 0) quatDiff = quatDiff.scaled(-1);
        Kore::RotationUtility::quatToEuler(&quatDiff, &deltaRot.x(), &deltaRot.y(), &deltaRot.z());
        
        deltaP[3] = deltaRot.x();
        if (nDOFs > 4) deltaP[4] = deltaRot.y();
        if (nDOFs > 5) deltaP[5] = deltaRot.z();
    }
    
    return deltaP;
}

Jacobian::mat_mxn Jacobian::calcJacobian() {
    Jacobian::mat_mxn jacobianMatrix;
    BoneNode* targetBone = endEffektor;
    
    // Get current rotation and position of the end-effector
    Kore::vec3 p_aktuell = getPosition(targetBone);
    
    int joint = 0;
    while (targetBone->initialized) {
        Kore::vec3 axes = targetBone->axes;
        
        if (axes.x() == 1.0) {
            vec_m column = calcJacobianColumn(targetBone, p_aktuell, Kore::vec3(1, 0, 0));
            for (int i = 0; i < nDOFs; ++i) jacobianMatrix[i][joint] = column[i];
            joint += 1;
        }
        if (axes.y() == 1.0) {
            vec_m column = calcJacobianColumn(targetBone, p_aktuell, Kore::vec3(0, 1, 0));
            for (int i = 0; i < nDOFs; ++i) jacobianMatrix[i][joint] = column[i];
            joint += 1;
        }
        if (axes.z() == 1.0) {
            vec_m column = calcJacobianColumn(targetBone, p_aktuell, Kore::vec3(0, 0, 1));
            for (int i = 0; i < nDOFs; ++i) jacobianMatrix[i][joint] = column[i];
            joint += 1;
        }
        
        targetBone = targetBone->parent;
    }
    
    // fill empty columns with zeros
    for (int j = joint; j < nJointDOFs; ++j)
        for (int i = 0; i < nDOFs; ++i)
            jacobianMatrix[i][j] = 0;
    
    return jacobianMatrix;
}

Jacobian::vec_m Jacobian::calcJacobianColumn(BoneNode* bone, Kore::vec3 p_aktuell, Kore::vec3 rotAxis) {
    vec_m column;
    
    // Get rotation and position vector of the current bone
    Kore::vec3 p_j = getPosition(bone);
    
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

Jacobian::mat_nxm Jacobian::calcPseudoInverse(Jacobian::mat_mxn jacobian, float lambda) { // lambda != 0 => DLS!
    if (nDOFs <= nJointDOFs) { // m <= n
        // Left Damped pseudo-inverse
        return (jacobian.Transpose() * jacobian + Jacobian::mat_nxn::Identity() * lambda * lambda).Invert() * jacobian.Transpose();
    }
    else {
        // Right Damped pseudo-inverse
        return jacobian.Transpose() * (jacobian * jacobian.Transpose() + Jacobian::mat_mxm::Identity() * lambda * lambda).Invert();
    }
}

Kore::vec3 Jacobian::getPosition(BoneNode* bone) {
    Kore::vec3 result;
    
    // from quat to euler!
    Kore::vec4 quat = bone->combined * Kore::vec4(0, 0, 0, 1);
    quat *= 1.0 / quat.w();
    
    result.x() = quat.x();
    result.y() = quat.y();
    result.z() = quat.z();
    
    return result;
}
