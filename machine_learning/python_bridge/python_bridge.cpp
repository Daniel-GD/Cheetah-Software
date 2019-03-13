#include <iostream>
#include <stdio.h>
#include <lcm/lcm-cpp.hpp>
#include <cpp/simulation_to_python.hpp>
#include <cpp/python_to_simulation.hpp>
#include <unistd.h>
#include <Utilities/Timer.h>

class Handler 
{
    public:
        ~Handler() {}

        void handleMessage(const lcm::ReceiveBuffer* rbuf,
                const std::string& chan, 
                const simulation_to_python * msg)
        {
            printf("Received message on channel \"%s\":\n", chan.c_str());
        }
};

int step(double in)
{
    printf("Python Bridge\n");
    lcm::LCM lcm_subscribe;
    lcm::LCM lcm_publish;

    if(!lcm_subscribe.good())
        return 1;

    python_to_simulation input;
    Handler handlerObject;

    Timer time;

    lcm_subscribe.subscribe("simulation_to_python", &Handler::handleMessage, &handlerObject);
    input.reset_call = false;
    input.jpos_cmd[0] = in;
    lcm_publish.publish("python_to_simulation", &input);
    lcm_subscribe.handle();
    
    printf("%f ms\n", time.getMs());
    return 0;
}
