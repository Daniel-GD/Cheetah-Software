#ifndef BOUNDING_CTRL
#define BOUNDING_CTRL

#include <FSM_States/ControlFSMData.h>
#include <Dynamics/FloatingBaseModel.h>
#include <Dynamics/Quadruped.h>
#include <ParamHandler/ParamHandler.hpp>
#include <lcm-cpp.hpp>
#include <WBC/WBIC/WBIC.hpp>
#include <WBC/WBLC/KinWBC.hpp>


#include "ImpulseCurve.hpp"
#include "wbc_test_data_t.hpp"

class MIT_UserParameters;

template <typename T>
class BoundingCtrl{
 public:
  BoundingCtrl(FloatingBaseModel<T> );
  ~BoundingCtrl();

  void FirstVisit();

  void run(ControlFSMData<T> & data);

 protected:
  void _body_task_setup();
  void _leg_task_setup();
  void _contact_update();
  void _compute_torque_wbic(DVec<T>& gamma);

  void _StatusCheck();
  void _setupTaskAndContactList();
  void _ContactUpdate();

  void _lcm_data_sending();
  void _save_file();
  void _UpdateModel(const StateEstimate<T> & state_est, 
      const LegControllerData<T> * leg_data);



  T _curr_time;
  T dt;
  std::vector<T> _Kp_joint, _Kd_joint;

  bool _b_front_swing;
  bool _b_hind_swing;

  bool _b_front_contact_est;
  bool _b_hind_contact_est;

  T _step_width;

  T _contact_vel_threshold;
  T _K_time;
  T _K_pitch;
  T _impact_amp;
  T _swing_height;

  T _aerial_duration;
  T _front_swing_time;

  T _front_previous_stance;
  T _front_previous_swing;
  T _front_current_stance;

  T _hind_previous_stance;
  T _hind_previous_swing;
  T _hind_current_stance;

  T _nominal_gait_period;
  T _gait_period;

  T _swing_time;
  T _stance_time;
  T _nominal_stance_time;
  int _dim_contact;

  T _front_start_time;
  T _hind_start_time;

  T _front_time;
  T _hind_time;

  T _step_length_lim;

  Task<T>* _local_head_pos_task;
  Task<T>* _local_tail_pos_task;

  Task<T>* _local_roll_task;
  Task<T>* _body_ryrz_task;

  Task<T>* _fr_foot_local_task;
  Task<T>* _fl_foot_local_task;
  Task<T>* _hr_foot_local_task;
  Task<T>* _hl_foot_local_task;

  Vec3<T> _fr_foot_pos;
  DVec<T> _fr_foot_vel;
  DVec<T> _fr_foot_acc;

  Vec3<T> _fl_foot_pos;
  DVec<T> _fl_foot_vel;
  DVec<T> _fl_foot_acc;

  Vec3<T> _hr_foot_pos;
  DVec<T> _hr_foot_vel;
  DVec<T> _hr_foot_acc;

  Vec3<T> _hl_foot_pos;
  DVec<T> _hl_foot_vel;
  DVec<T> _hl_foot_acc;

  ContactSpec<T>* _fr_contact;
  ContactSpec<T>* _fl_contact;
  ContactSpec<T>* _hr_contact;
  ContactSpec<T>* _hl_contact;

  WBIC<T>* _wbic;
  WBIC_ExtraData<T>* _wbic_data;

  T _target_leg_height;
  T _total_mass;

  ImpulseCurve<T> _front_z_impulse;
  ImpulseCurve<T> _hind_z_impulse;

  Vec3<T> _ini_front_body;
  Vec3<T> _ini_hind_body;

  Vec3<T> _ini_fr;
  Vec3<T> _ini_fl;
  Vec3<T> _ini_hr;
  Vec3<T> _ini_hl;

  Vec3<T> _fin_fr;
  Vec3<T> _fin_fl;
  Vec3<T> _fin_hr;
  Vec3<T> _fin_hl;

  Vec3<T> _vel_des;

  DVec<T> _Fr_des;

  DVec<T> _des_jpos;
  DVec<T> _des_jvel;
  DVec<T> _des_jacc;

  KinWBC<T>* _kin_wbc;
  std::vector<Task<T>*> _task_list;
  std::vector<ContactSpec<T>*> _contact_list;

  ParamHandler* _param_handler;
  lcm::LCM _wbcLCM;
  wbc_test_data_t _wbc_data_lcm;

  FloatingBaseModel<T> _model;
  FBModelState<T> _state;
  DVec<T> _full_config;
  DMat<T> _A;
  DMat<T> _Ainv;
  DVec<T> _grav;
  DVec<T> _coriolis;

};

#endif
