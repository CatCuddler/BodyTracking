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
    Jacobian(BoneNode* bone, Kore::vec4 pos, Kore::Quaternion rot);
    float getError();
    std::vector<float> getThetaByPseudoInverse();
    std::vector<float> getThetaByDLS(float lambda = 1.0);
    
private:
    static const int nDOFs = 6; // m = 3k, 3 position + 3 orientation
    static const int nGelenke = 3; // n
    // static const int nJointDOFs = 3;
    
    typedef Kore::Matrix<nGelenke, nDOFs, float> mat_mxn;
    typedef Kore::Matrix<nDOFs, nGelenke, float> mat_nxm;
    typedef Kore::Matrix<nDOFs, nDOFs, float> mat_mxm;
    typedef Kore::Matrix<nGelenke, nGelenke, float> mat_nxn;
    typedef Kore::Vector<float, nDOFs> vec_m;
    typedef Kore::Vector<float, nGelenke> vec_n;
    
    BoneNode* endEffektor;
    Kore::vec4 pos_soll;
    Kore::Quaternion rot_soll;
    vec_m deltaP;
    
    /*
    mat_mxn J_target;
    mat_mxm U; // SVD
    mat_mxn D; // SVD
    mat_nxn V; // SVD */
    
    vec_m calcDeltaP();
    mat_mxn calcJacobian(Kore::vec4 rotAxis);
    mat_nxm calcPseudoInverse(Jacobian::mat_mxn jacobian, float lambda);
};

#endif /* Jacobian_h */
