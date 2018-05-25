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

Jacobian::Jacobian(BoneNode* bone, Kore::vec4 pos, Kore::Quaternion rot) {
    endEffektor = bone;
    pos_soll = pos;
    rot_soll = rot;
}

float Jacobian::getError() {
    return (!deltaP.isZero() ? deltaP : calcDeltaP()).getLength();
}

std::vector<float> Jacobian::getThetaByPseudoInverse() {
    return getThetaByDLS(0.5);
}

std::vector<float> Jacobian::getThetaByDLS(float lambda) {
    std::vector<float> theta;
    
    // Get Jacobian
    mat_mxn jacobianX = calcJacobian(Kore::vec4(1, 0, 0, 0));
    mat_mxn jacobianY = calcJacobian(Kore::vec4(0, 1, 0, 0));
    mat_mxn jacobianZ = calcJacobian(Kore::vec4(0, 0, 1, 0));
    
    // Get Pseudo Inverse
    mat_nxm pseudoInvX = calcPseudoInverse(jacobianX, lambda);
    mat_nxm pseudoInvY = calcPseudoInverse(jacobianY, lambda);
    mat_nxm pseudoInvZ = calcPseudoInverse(jacobianZ, lambda);
     
    // Get deltaP
    deltaP = calcDeltaP();
    
    // Calculate the angles
    vec_n aThetaX = pseudoInvX * deltaP;
    vec_n aThetaY = pseudoInvY * deltaP;
    vec_n aThetaZ = pseudoInvZ * deltaP;
    
    for (int i = 0; i < nGelenke; ++i) {
        theta.push_back(aThetaX[i]);
        theta.push_back(aThetaY[i]);
        theta.push_back(aThetaZ[i]);
    }
    
    return theta;
}

Jacobian::vec_m Jacobian::calcDeltaP() {
    vec_m deltaP;
    
    // Calculate difference between desired position and actual position of the end effector
    Kore::vec4 pos_aktuell = endEffektor->combined * Kore::vec4(0, 0, 0, 1);
    pos_aktuell *= 1.0 / pos_aktuell.w();
    Kore::vec4 deltaPos = pos_soll - pos_aktuell;
    
    // Calculate difference between desired rotation and actual rotation
    Kore::vec3 deltaRot = Kore::vec3(0, 0, 0);
    Kore::Quaternion rot_aktuell;
    Kore::RotationUtility::getOrientation(&endEffektor->combined, &rot_aktuell);
    Kore::Quaternion desQuat = rot_soll;
    desQuat.normalize();
    Kore::Quaternion quatDiff = desQuat.rotated(rot_aktuell.invert());
    if (quatDiff.w < 0) quatDiff = quatDiff.scaled(-1);
    Kore::RotationUtility::quatToEuler(&quatDiff, &deltaRot.x(), &deltaRot.y(), &deltaRot.z());
    
    // position
    deltaP[0] = deltaPos.x();
    deltaP[1] = deltaPos.y();
    deltaP[2] = deltaPos.z();
    // orientation
    deltaP[3] = deltaRot.x();
    deltaP[4] = deltaRot.y();
    deltaP[5] = deltaRot.z();
    
    return deltaP;
}

Jacobian::mat_mxn Jacobian::calcJacobian(Kore::vec4 rotAxis) {
    Jacobian::mat_mxn jacobianMatrix;
    BoneNode* targetBone = endEffektor;
    
    // Get current rotation and position of the end-effector
    Kore::vec4 p_aktuell = targetBone->combined * Kore::vec4(0, 0, 0, 1);
    p_aktuell *= 1.0 / p_aktuell.w();
    
    for (int b = 0; b < nGelenke; ++b) {
        Kore::vec4 p_j, v_j, cross;
        BoneNode* bone = targetBone;
        
        Kore::vec3 axes = bone->axes;
        if ((axes.x() == 1 && rotAxis.x() == 1) || (axes.y() == 1 && rotAxis.y() == 1) || (axes.z() == 1 && rotAxis.z() == 1)) {
            // Get rotation and position vector of the current bone
            p_j = bone->combined * Kore::vec4(0, 0, 0, 1);
            p_j *= 1.0 / p_j.w();
            
            v_j = bone->combined * rotAxis;
            
            // Calculate cross product
            cross = v_j.cross(p_aktuell - p_j);
        }
        
        for (int i = 0; i < nDOFs; ++i) {
            switch(i) {
                case 0 : jacobianMatrix[i][b] = cross.x();
                    break;
                case 1 : jacobianMatrix[i][b] = cross.y();
                    break;
                case 2 : jacobianMatrix[i][b] = cross.z();
                    break;
                case 3 : jacobianMatrix[i][b] = v_j.x();
                    break;
                case 4 : jacobianMatrix[i][b] = v_j.y();
                    break;
                case 5 : jacobianMatrix[i][b] = v_j.z();
                    break;
            }
        }
        
        targetBone = targetBone->parent;
    }
    
    return jacobianMatrix;
}

Jacobian::mat_nxm Jacobian::calcPseudoInverse(Jacobian::mat_mxn jacobian, float lambda) { // lambda != 0 => DLS!
    if (nDOFs <= nGelenke) { // m <= n
        // Left Damped pseudo-inverse
        return (jacobian.Transpose() * jacobian + Jacobian::mat_nxn::Identity() * lambda * lambda).Invert() * jacobian.Transpose();
    }
    else {
        // Right Damped pseudo-inverse
        return jacobian.Transpose() * (jacobian * jacobian.Transpose() + Jacobian::mat_mxm::Identity() * lambda * lambda).Invert();
    }
}
