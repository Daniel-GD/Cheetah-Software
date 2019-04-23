#ifndef BACKFLIP_CTRL
#define BACKFLIP_CTRL

#include <WBC_States/Controller.hpp>
#include <ParamHandler/ParamHandler.hpp>

template <typename T> class ContactSpec;
template <typename T> class WBLC;
template <typename T> class WBLC_ExtraData;
template <typename T> class KinWBC;
template <typename T> class StateProvider;
template <typename T> class WBLCTrotTest;

class DataReader;

template <typename T>
class BackFlipCtrl: public Controller<T>{
    public:
        BackFlipCtrl(const FloatingBaseModel<T>* , DataReader* );
        virtual ~BackFlipCtrl();

        virtual void OneStep(void* _cmd);
        virtual void FirstVisit();
        virtual void LastVisit();
        virtual bool EndOfPhase();

        virtual void CtrlInitialization(const std::string & category_name);
        virtual void SetTestParameter(const std::string & test_file);

    protected:
        DataReader* _data_reader;

        DVec<T> _Kp, _Kd;
        DVec<T> _des_jpos; 
        DVec<T> _des_jvel; 
        DVec<T> _jtorque; 

        bool _b_set_height_target;
        T _end_time;
        int _dim_contact;

        void _update_joint_command();

        T _ctrl_start_time;
        ParamHandler* _param_handler;
        StateProvider<T>* _sp;
};

#endif
