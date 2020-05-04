#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <limits>
#include <math.h>
#include <assert.h>
#include <memory>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <algorithm>
#include <limits>
#include <random>
#include <stdio.h>

#ifdef RT_ARM_MBED

#include "../mbed.h"

using namespace cadmium;
using namespace std;

//Convert the int to string to be used in 7-seg disp
std::string time_to_str(int hour, int minute){
    string intToCharMin = to_string(minute);
    string intToCharHour = to_string(hour);

    if (minute > -1 && minute < 10)
        intToCharMin = "0" + intToCharMin;

    if (hour > -1 && hour < 10)
        intToCharHour = "0" + intToCharHour;

    return(intToCharHour + intToCharMin); 
    }

//Port definition
struct alarm_clock_defs{
    struct in : public in_port<bool> {}; //start / stop clock button
    struct setHour : public in_port<bool> {}; //set hour
    struct setMin : public in_port<bool>{}; //set min

    struct out : public out_port<string> {}; //output to the display
};

template <typename TIME> class alarm_clock {
    using defs=alarm_clock_defs; // putting definitions in context

public:
    alarm_clock() noexcept{
        state.running = false;
        //TIME("00:00:05:00");
        //paused = std::numeric_limits<TIME>::infinity();
        //state.running = false;
        state.ext_out = 0; 
        state.minute = 0;
        state.hour = 0;
        state.output = "0000";
    }

    // state definition
    struct state_type{
        int minute;
        int hour;
        std::string output;
        bool running;
        int ext_out;
    };
    state_type state;

    // ports definition
    using input_ports=std::tuple<typename defs::in, defs::setHour, defs::setMin>;
    using output_ports=std::tuple<typename defs::out>;

    // internal transition
    void internal_transition() {
        if(state.ext_out == 2)
            state.hour ++;
        else
            state.minute ++;

        state.ext_out = 0;

        if(state.minute >= 60){
            state.hour += 1;
            state.minute = 0;
        }
        if(state.hour >= 24){
            state.hour = 0;
            state.minute = 0;
        }

        state.output = time_to_str(state.hour, state.minute);
    }
    // external transition
    void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        for(const auto &x : get_messages<typename defs::in>(mbs)){
            if(x)
                state.running =!state.running;
        }
        for(const auto &x : get_messages<typename defs::setMin>(mbs)){
            if(x == true && state.running == false && state.minute < 60)
                state.ext_out = 1;
                //state.minute ++;
                //state.output = time_to_str(state.hour, state.minute);
        } 
        for(const auto &x : get_messages<typename defs::setHour>(mbs)){
            if(x == true && state.running == false && state.hour < 24)
                state.ext_out = 2;
                //state.hour ++;
                //state.output = time_to_str(state.hour, state.minute);
        }   
                               
    }

    // confluence transition
    void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        external_transition(TIME(), std::move(mbs));
        internal_transition();
    }

    // output function
    typename make_message_bags<output_ports>::type output() const {
        typename make_message_bags<output_ports>::type bags;

        //state.output = time_to_str(state.hour, state.minute);
        get_messages<typename defs::out>(bags).push_back(state.output);

        return bags;
    }

    // time_advance function
    TIME time_advance() const {

        TIME running;

        if(state.ext_out != 0)
            running = TIME("00:00:00:00");
        else if(state.running)
            running = TIME("00:00:01:00");
        else
            running = std::numeric_limits<TIME>::infinity();

        return running;
    }

    friend std::ostringstream& operator<<(std::ostringstream& os, const typename alarm_clock<TIME>::state_type& i) {
        os << "Printed: " << "1";//i.output;
        return os;
    }
};


/******************************************************************************
* SIMULATOR IMPLEMENTATION
*****************************************************************************/
#else

#include <cadmium/io/oestream.hpp>

using namespace cadmium;
using namespace std;

//Output file for LCD
const char* ALARM_CLOCK_FILE = "./outputs/alarm_clock.txt";

//Port definition
struct alarm_clock_defs{
    struct in : public in_port<bool> {}; //start / stop clock button
    struct setHour : public in_port<bool> {}; //set hour
    struct setMin : public in_port<bool>{}; //set min

    struct out : public out_port<string> {}; //output to the display
};

template<typename TIME>
class alarm_clock : public oestream_output<string, TIME, alarm_clock_defs>{
public:
    alarm_clock() : oestream_output<string, TIME, alarm_clock_defs>(ALARM_CLOCK_FILE) {}
};

#endif //RT_ARM_MBED
