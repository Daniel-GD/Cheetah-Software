#include "LocomotionCtrl.hpp"
#include <WBC_Ctrl/ContactSet/SingleContact.hpp>
#include <WBC_Ctrl/TaskSet/BodyOriTask.hpp>
#include <WBC_Ctrl/TaskSet/BodyPosTask.hpp>
#include <WBC_Ctrl/TaskSet/LinkPosTask.hpp>

template<typename T>
LocomotionCtrl<T>::LocomotionCtrl(FloatingBaseModel<T> model):
  WBC_Ctrl<T>(model)
{
  _body_pos_task = new BodyPosTask<T>(&(WBCtrl::_model));
  _body_ori_task = new BodyOriTask<T>(&(WBCtrl::_model));


  _foot_contact[0] = new SingleContact<T>(&(WBCtrl::_model), linkID::FR);
  _foot_contact[1] = new SingleContact<T>(&(WBCtrl::_model), linkID::FL);
  _foot_contact[2] = new SingleContact<T>(&(WBCtrl::_model), linkID::HR);
  _foot_contact[3] = new SingleContact<T>(&(WBCtrl::_model), linkID::HL);

  _foot_task[0] = new LinkPosTask<T>(&(WBCtrl::_model), linkID::FR);
  _foot_task[1] = new LinkPosTask<T>(&(WBCtrl::_model), linkID::FL);
  _foot_task[2] = new LinkPosTask<T>(&(WBCtrl::_model), linkID::HR);
  _foot_task[3] = new LinkPosTask<T>(&(WBCtrl::_model), linkID::HL);
}

template<typename T>
LocomotionCtrl<T>::~LocomotionCtrl(){
  delete _body_pos_task;
  delete _body_ori_task;

  for(size_t i (0); i<4; ++i){
    delete _foot_contact[i];
    delete _foot_task[i];
  }
}

template<typename T>
void LocomotionCtrl<T>::_ContactTaskUpdate(void* input, ControlFSMData<T> & data){
  _input_data = static_cast<LocomotionCtrlData<T>* >(input);

  _ParameterSetup(data.userParameters);
  
  // Wash out the previous setup
  _CleanUp();

  // Task & Contact Set
  Vec3<T> zero_vec3; zero_vec3.setZero();

  _quat_des = ori::rpyToQuat(_input_data->pBody_RPY_des);
  _body_ori_task->UpdateTask(&_quat_des, _input_data->vBody_Ori_des, zero_vec3);
  _body_pos_task->UpdateTask(
      &(_input_data->pBody_des), 
      _input_data->vBody_des, 
      _input_data->aBody_des);

  WBCtrl::_task_list.push_back(_body_ori_task);
  WBCtrl::_task_list.push_back(_body_pos_task);

  for(size_t leg(0); leg<4; ++leg){
    if(_input_data->contact_state[leg] > 0.){ // Contact
      _foot_contact[leg]->setRFDesired((DVec<T>)(_input_data->Fr_des[leg]));
      _foot_contact[leg]->UpdateContactSpec();
      WBCtrl::_contact_list.push_back(_foot_contact[leg]);

    }else{ // No Contact (swing)
      _foot_task[leg]->UpdateTask(
          &(_input_data->pFoot_des[leg]), 
          _input_data->vFoot_des[leg], 
          _input_data->aFoot_des[leg]);
      WBCtrl::_task_list.push_back(_foot_task[leg]);
    }
  }
}

template<typename T>
void LocomotionCtrl<T>::_ParameterSetup(const MIT_UserParameters* param){
  WBCtrl::_wbic_data->_W_floating = DVec<T>::Constant(6, param->wbc_base_Fr_weight);

  for(size_t i(0); i<3; ++i){
    ((BodyPosTask<T>*)_body_pos_task)->_Kp[i] = param->Kp_body[i];
    ((BodyPosTask<T>*)_body_pos_task)->_Kd[i] = param->Kp_body[i];

    ((BodyOriTask<T>*)_body_ori_task)->_Kp[i] = param->Kp_ori[i];
    ((BodyOriTask<T>*)_body_ori_task)->_Kd[i] = param->Kp_ori[i];

    WBCtrl::_Kp_joint[i] = param->Kp_joint[i];
    WBCtrl::_Kd_joint[i] = param->Kd_joint[i];
  }
}


template<typename T>
void LocomotionCtrl<T>::_CleanUp(){
  WBCtrl::_contact_list.clear();
  WBCtrl::_task_list.clear();
}

template<typename T>
void LocomotionCtrl<T>::_LCM_PublishData() {
  int iter(0);
  for(size_t leg(0); leg<4; ++leg){
    _Fr_result[leg].setZero();
    
    if(_input_data->contact_state[leg]>0.){
      for(size_t i(0); i<3; ++i){
        _Fr_result[leg][i] = WBCtrl::_wbic_data->_Fr[3*iter + i];
      }
      ++iter;
    }
  }

  for(size_t i(0); i<3; ++i){
    WBCtrl::_wbc_data_lcm.foot_pos[i] = WBCtrl::_model._pGC[linkID::FR][i];
    WBCtrl::_wbc_data_lcm.foot_vel[i] = WBCtrl::_model._vGC[linkID::FR][i];

    WBCtrl::_wbc_data_lcm.foot_pos[i + 3] = WBCtrl::_model._pGC[linkID::FL][i];
    WBCtrl::_wbc_data_lcm.foot_vel[i + 3] = WBCtrl::_model._vGC[linkID::FL][i];

    WBCtrl::_wbc_data_lcm.foot_pos[i + 6] = WBCtrl::_model._pGC[linkID::HR][i];
    WBCtrl::_wbc_data_lcm.foot_vel[i + 6] = WBCtrl::_model._vGC[linkID::HR][i];

    WBCtrl::_wbc_data_lcm.foot_pos[i + 9] = WBCtrl::_model._pGC[linkID::HL][i];
    WBCtrl::_wbc_data_lcm.foot_vel[i + 9] = WBCtrl::_model._vGC[linkID::HL][i];


    for(size_t leg(0); leg<4; ++leg){
      WBCtrl::_wbc_data_lcm.Fr_des[3*leg + i] = _input_data->Fr_des[leg][i];
      WBCtrl::_wbc_data_lcm.Fr[3*leg + i] = _Fr_result[leg][i];

      WBCtrl::_wbc_data_lcm.foot_pos_cmd[3*leg + i] = _input_data->pFoot_des[leg][i];
      WBCtrl::_wbc_data_lcm.foot_vel_cmd[3*leg + i] = _input_data->vFoot_des[leg][i];
      WBCtrl::_wbc_data_lcm.foot_acc_cmd[3*leg + i] = _input_data->aFoot_des[leg][i];

      WBCtrl::_wbc_data_lcm.jpos_cmd[3*leg + i] = WBCtrl::_des_jpos[3*leg + i];
      WBCtrl::_wbc_data_lcm.jvel_cmd[3*leg + i] = WBCtrl::_des_jvel[3*leg + i];
      WBCtrl::_wbc_data_lcm.jacc_cmd[3*leg + i] = WBCtrl::_des_jacc[3*leg + i];

      WBCtrl::_wbc_data_lcm.jpos[3*leg + i] = WBCtrl::_state.q[3*leg + i];
      WBCtrl::_wbc_data_lcm.jvel[3*leg + i] = WBCtrl::_state.qd[3*leg + i];
    }

    WBCtrl::_wbc_data_lcm.body_pos_cmd[i] = _input_data->pBody_des[i];
    WBCtrl::_wbc_data_lcm.body_vel_cmd[i] = _input_data->vBody_des[i];
    WBCtrl::_wbc_data_lcm.body_ori_cmd[i] = _quat_des[i];

    WBCtrl::_wbc_data_lcm.body_pos[i] = WBCtrl::_state.bodyPosition[i];
    WBCtrl::_wbc_data_lcm.body_vel[i] = WBCtrl::_state.bodyVelocity[i+3];
    WBCtrl::_wbc_data_lcm.body_ori[i] = WBCtrl::_state.bodyOrientation[i];

    WBCtrl::_wbc_data_lcm.body_ang_vel[i] = WBCtrl::_state.bodyVelocity[i];
  }
  WBCtrl::_wbc_data_lcm.body_ori_cmd[3] = _quat_des[3];
  WBCtrl::_wbc_data_lcm.body_ori[3] = WBCtrl::_state.bodyOrientation[3];

  WBCtrl::_wbcLCM.publish("wbc_lcm_data", &(WBCtrl::_wbc_data_lcm) );
}

template class LocomotionCtrl<float>;
template class LocomotionCtrl<double>;
