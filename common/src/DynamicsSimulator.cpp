#include "DynamicsSimulator.h"


/*!
 * Initialize the dynamics simulator by allocating memory for ABA matrices
 */
template <typename T>
DynamicsSimulator<T>::DynamicsSimulator(FloatingBaseModel<T> &model) :_model(model) {
  // initialize the ABA quantities:
  _nb = _model._nDof;

  // allocate matrices
  _Xup.resize(_nb);
  _Xuprot.resize(_nb);
  _IA.resize(_nb);
  _v.resize(_nb);
  _vrot.resize(_nb);
  _a.resize(_nb);
  _c.resize(_nb);
  _crot.resize(_nb);
  _U.resize(_nb);
  _Urot.resize(_nb);
  _Utot.resize(_nb);
  _S.resize(_nb);
  _Srot.resize(_nb);
  _pA.resize(_nb);
  _pArot.resize(_nb);
  _d.resize(_nb);
  _u.resize(_nb);

  // set stuff for 0:5
  for(size_t i = 0; i < 6; i++) {
    _Xup[i] = Mat6<T>::Identity();
    _Xuprot[i] = Mat6<T>::Identity();
    _v[i] = SVec<T>::Zero();
    _vrot[i] = SVec<T>::Zero();
    _a[i] = SVec<T>::Zero();
    _c[i] = SVec<T>::Zero();
    _crot[i] = SVec<T>::Zero();
    _U[i] = SVec<T>::Zero();
    _Urot[i] = SVec<T>::Zero();
    _Utot[i] = SVec<T>::Zero();
    _S[i] = SVec<T>::Zero();
    _Srot[i] = SVec<T>::Zero();
    _d[i] = 0;
    _u[i] = 0;
    _pA[i] = SVec<T>::Zero();
    _pArot[i] = SVec<T>::Zero();
  }

  _state.bodyVelocity = SVec<T>::Zero();
  _state.bodyPosition = Vec3<T>::Zero();
  _state.bodyOrientation = Quat<T>::Zero();
  // this is a hack to get tests to run for now
  _state.q = DVec<T>::Zero(12);
  _state.qd = DVec<T>::Zero(12);
}

template <typename T>
void DynamicsSimulator<T>::step(T dt, const DVec<T> &tau, const ForceList<T> *externalForces) {
  (void)dt;
  _externalForces = externalForces;

  updateCollisions();
  updateGroundForces();
  runABA(tau);
  integrate();
}

template <typename T>
void DynamicsSimulator<T>::updateCollisions() {

}

template <typename T>
void DynamicsSimulator<T>::updateGroundForces() {

}

template <typename T>
void DynamicsSimulator<T>::integrate() {

}

/*!
 * Articulated Body Algorithm, modified by Pat to add rotors
 */
template <typename T>
void DynamicsSimulator<T>::runABA(const DVec<T> &tau) {
  // create spatial vector for gravity
  SVec<T> aGravity;
  aGravity << 0, 0, 0, _model._gravity[0], _model._gravity[1], _model._gravity[2];

  // calc coordinate transformation for float-base
  // TODO, check the transpose on the rotation matrix
  _Xup[5] = createSXform(quaternionToRotationMatrix(_state.bodyOrientation).transpose(), _state.bodyPosition);
  // float-base velocity
  _v[5] = _state.bodyVelocity;
  // float-base articulated inertia
  _IA[5] = _model._Ibody[5].getMatrix();
  SVec<T> ivProduct = _model._Ibody[5].getMatrix() * _v[5];
  _pA[5] = forceCrossProduct(_v[5], ivProduct);

  // loop 1, down the tree
  for(size_t i = 6; i < _nb; i++) {
    // joint xform
    Mat6<T> XJ = jointXform(_model._jointTypes[i], _model._jointAxes[i], _state.q[i - 6]);

    // spatial velocity due to qd
    _S[i] = jointMotionSubspace<T>(_model._jointTypes[i], _model._jointAxes[i]);
    SVec<T> vJ = _S[i] * _state.qd[i - 6];

    // coordinate transformation from up the tree
    _Xup[i] = XJ * _model._Xtree[i];

    // total velocity of body i
    _v[i] = _Xup[i] * _v[_model._parents[i]] + vJ;
    _c[i] = motionCrossProduct(_v[i], vJ);


    _IA[i] = _model._Ibody[i].getMatrix(); // initialize
    ivProduct = _model._Ibody[i].getMatrix() * _v[i];
    _pA[i] = forceCrossProduct(_v[i], ivProduct);

    // same for rotors
    Mat6<T> XJrot = jointXform(_model._jointTypes[i], _model._jointAxes[i], _state.q[i - 6] * _model._gearRatios[i]);
    _Srot[i] = _S[i] * _model._gearRatios[i];
    SVec<T> vJrot = _Srot[i] * _state.qd[i - 6];
    _Xuprot[i] = XJrot * _model._Xrot[i];
    _vrot[i] = _Xuprot[i] * _v[_model._parents[i]] + vJrot;
    _crot[i] = motionCrossProduct(_vrot[i], vJrot);
    ivProduct = _model._Irot[i].getMatrix() * _vrot[i];
    _pArot[i] = forceCrossProduct(_vrot[i], ivProduct);
  }

  // adjust pA for external forces
  if(_externalForces) {
    for(size_t i = 5; i < _nb; i++) {
      if(_model._parents[i] == 0) {
        _Xa[i] = _Xup[i]; // float base
      } else {
        _Xa[i] = _Xup[i] * _Xa[_model._parents[i]];
      }
      // TODO add if statement
      Mat3<T> R = rotationFromSXform(_Xa[i]);
      Vec3<T> p = translationFromSXform(_Xa[i]);
      Mat6<T> iX = createSXform(R.transpose(), - R * p);
      _pA[i] = _pA[i] - iX.transpose() * _externalForces->at(i);
    }
  }

  // Pat's magic principle of least constraint
  for(size_t i = _nb - 1; i >= 6; i--) {
    _U[i] = _IA[i] * _S[i];
    _Urot[i] = _model._Irot[i].getMatrix() * _Srot[i];
    _Utot[i] = _Xup[i].transpose() * _U[i] + _Xuprot[i].transpose() * _Urot[i];

    _d[i] = _Srot[i].transpose() * _Urot[i];
    _d[i] += _S[i].transpose() * _U[i];
    _u[i] = tau[i - 6] - _S[i].transpose() * _pA[i] - _Srot[i].transpose() * _pArot[i] - _U[i].transpose()*_c[i] - _Urot[i].transpose() * _crot[i];

    // articulated inertia recursion
    Mat6<T> Ia = _Xup[i].transpose() * _IA[i] * _Xup[i] + _Xuprot[i].transpose() * _model._Irot[i].getMatrix() * _Xuprot[i] - _Utot[i] * _Utot[i].transpose() / _d[i];
    _IA[_model._parents[i]] += Ia;
    SVec<T> pa = _Xup[i].transpose() * (_pA[i] + _IA[i] * _c[i]) + _Xuprot[i].transpose() * (_pArot[i] + _model._Irot[i].getMatrix() * _crot[i]) + _Utot[i] * _u[i] / _d[i];
    _pA[_model._parents[i]] += pa;
  }


  // include gravity and compute acceleration of floating base
  SVec<T> a0 = -aGravity;
  SVec<T> ub = -_pA[5];
  _a[5] = _Xup[5] * a0;
  SVec<T> afb = _IA[5].colPivHouseholderQr().solve(ub - _IA[5].transpose() * _a[5]);
  _a[5] += afb;

  // joint accelerations
  _dstate.qdd = DVec<T>(_nb - 6);
  for(size_t i = 6; i < _nb; i++) {
    _dstate.qdd[i - 6] = (_u[i] - _Utot[i].transpose() * _a[_model._parents[i]]) / _d[i];
    _a[i] = _Xup[i] * _a[_model._parents[i]] + _S[i] * _dstate.qdd[i-6] + _c[i];
  }

  // output
  RotMat<T> Rup = rotationFromSXform(_Xup[5]);
  _dstate.dQuat = quatDerivative(_state.bodyOrientation, _state.bodyVelocity.template block<3,1>(0,0));
  _dstate.dBasePosition = Rup.transpose() * _state.bodyVelocity.template block<3,1>(3,0);
  _dstate.dBaseVelocity = afb;
  // qdd is set in the for loop above

}

template class DynamicsSimulator<double>;
template class DynamicsSimulator<float>;