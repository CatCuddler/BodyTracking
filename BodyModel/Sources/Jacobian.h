//
//  Jacobian.h
//  BodyTracking
//
//  Created by Philipp Achenbach on 16.05.18.
//  Copyright © 2018 KTX Software Development. All rights reserved.
//

#ifndef Jacobian_h
#define Jacobian_h

#include <Kore/Math/Quaternion.h>
#include "MeshObject.h"

#include <vector>

struct BoneNode;

class Jacobian {
    
public:
    Jacobian(BoneNode* bone, Kore::vec4* pos, Kore::Quaternion* rot);
    float getError();
    std::vector<float> getThetaByPseudoInverse();
    
    static const int nDOFs = 6; // 3 position + 3 orientation
    static const int nJointDOFs = 3; // maxBone;
    typedef Kore::Matrix<nDOFs, nJointDOFs, float> mat_mxn;
    typedef Kore::Vector<float, nDOFs> vec_m;
    vec_m calcDeltaP();
    mat_mxn calcJacobian(Kore::vec4 rotAxis);
    mat_mxn calcPseudoInverse(Jacobian::mat_mxn jacobian, float lambda = 1);
    
private:
    // static const int nDOFs = 6; // 3 position + 3 orientation
    // static const int nJointDOFs = 3; // maxBone;
    
    // typedef Kore::Matrix<nDOFs, nJointDOFs, float> mat_mxn;
    typedef Kore::Matrix<nDOFs, nDOFs, float> mat_mxm;
    typedef Kore::Matrix<nJointDOFs, nJointDOFs, float> mat_nxn;
    // typedef Kore::Vector<float, nDOFs> vec_m;
    
    BoneNode* endEffektor;
    Kore::vec4* pos_soll;
    Kore::Quaternion* rot_soll;
    /*
    mat_mxn J_target;
    mat_mxm U; // SVD
    mat_mxn D; // SVD
    mat_nxn V; // SVD */
    
    // vec_m calcDeltaP();
    // mat_mxn calcJacobian(Kore::vec4 rotAxis);
    // mat_mxn calcPseudoInverse(Jacobian::mat_mxn jacobian, float lambda = 1);
};

#endif /* Jacobian_h */
