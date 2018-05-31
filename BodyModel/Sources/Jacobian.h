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
    static const int maxDOFs = 3;       // m = 3k, 3 position + 3 orientation
    static const int maxJointDOFs = 9;  // n, maximale Anzahl an Gelenke-Freiheitsgrade!
    
public:
    Jacobian(BoneNode* endEffektor, Kore::vec4 pos_soll, Kore::Quaternion rot_soll, int n = maxJointDOFs);
    float getError();
    std::vector<float> calcDeltaTheta(int ikMode = 0);
    
private:
    const float lambdaPseudoInverse = 0.1;  // Eigentlich 0, da sonst DLS! Bei 0 aber Stabilitätsprobleme!!!
    const float lambdaSVD = 0.12;           // Lambda für SVD
    const float lambdaDLS = 2.0;            // Lambda für DLS, 0.24 Optimum laut Buss
    const float lambdaDLSwithSVD = 1.0;     // Lambda für DLS with SVD
    const float lambdaSDLS = 0.7853981634;  // Lambda für SDLS = 45° * PI / 180°
    
    typedef Kore::Matrix<maxJointDOFs, maxDOFs, float>      mat_mxn;
    typedef Kore::Matrix<maxDOFs, maxJointDOFs, float>      mat_nxm;
    typedef Kore::Matrix<maxDOFs, maxDOFs, float>           mat_mxm;
    typedef Kore::Matrix<maxJointDOFs, maxJointDOFs, float> mat_nxn;
    typedef Kore::Vector<float, maxDOFs>                    vec_m;
    typedef Kore::Vector<float, maxJointDOFs>               vec_n;
    
    BoneNode*           endEffektor;
    Kore::vec3          pos_soll;
    Kore::Quaternion    rot_soll;
    float               error;
    int                 nDOFs = maxDOFs;
    int                 nJointDOFs;
    
    mat_mxm U;
    mat_nxn V;
    vec_m   d;
    
    vec_n       calcDeltaThetaByTranspose();
    vec_n       calcDeltaThetaByPseudoInverse();
    vec_n       calcDeltaThetaByDLS();
    vec_n       calcDeltaThetaBySVD();
    vec_n       calcDeltaThetaByDLSwithSVD();
    vec_n       calcDeltaThetaBySDLS();
    vec_m       calcDeltaP();
    mat_mxn     calcJacobian();
    vec_m       calcJacobianColumn(BoneNode* bone, Kore::vec3 p_aktuell, Kore::vec3 rotAxis);
    mat_nxm     calcPseudoInverse(float lambda);
    Kore::vec3  calcPosition(BoneNode* bone);
    void        calcSVD(Jacobian::mat_mxn jacobian);
    vec_n       clampMaxAbs(vec_n vec, float gamma_i);
    float       MaxAbs(vec_m vec);
    float       MaxAbs(vec_n vec, float start = 0);
};

#endif /* Jacobian_h */
