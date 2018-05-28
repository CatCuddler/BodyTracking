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
    static const int nDOFs = 3; // m = 3k, 3 position + 3 orientation
    static const int nJointDOFs = 9; // n, maximale Anzahl an Gelenke-Freiheitsgrade!
    
public:
    Jacobian(BoneNode* endEffektor, Kore::vec4 pos_soll, Kore::Quaternion rot_soll);
    float getError();
    
    typedef Kore::Vector<float, nJointDOFs> vec_n;
    
    vec_n calcDeltaThetaByTranspose();
    vec_n calcDeltaThetaByPseudoInverse();
    vec_n calcDeltaThetaByDLS();
    
private:
    typedef Kore::Matrix<nJointDOFs, nDOFs, float>      mat_mxn;
    typedef Kore::Matrix<nDOFs, nJointDOFs, float>      mat_nxm;
    typedef Kore::Matrix<nDOFs, nDOFs, float>           mat_mxm;
    typedef Kore::Matrix<nJointDOFs, nJointDOFs, float> mat_nxn;
    typedef Kore::Vector<float, nDOFs>                  vec_m;
    
    BoneNode*           endEffektor;
    Kore::vec3          pos_soll;
    Kore::Quaternion    rot_soll;
    vec_m               deltaP;
    
    mat_mxm U; // SVD
    mat_mxn D; // SVD
    mat_nxn V; // SVD
    
    vec_m       calcDeltaP();
    mat_mxn     calcJacobian();
    vec_m       calcJacobianColumn(BoneNode* bone, Kore::vec3 p_aktuell, Kore::vec3 rotAxis);
    mat_nxm     calcPseudoInverse(float lambda);
    Kore::vec3  calcPosition(BoneNode* bone);
};

#endif /* Jacobian_h */
