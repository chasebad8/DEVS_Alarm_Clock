#include <iostream>
#include <chrono>
#include <algorithm>
#include <string>

#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/concept/coupled_model_assert.hpp>
#include <cadmium/modeling/dynamic_coupled.hpp>
#include <cadmium/modeling/dynamic_atomic.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/tuple_to_ostream.hpp>
#include <cadmium/logger/common_loggers.hpp>

#include <NDTime.hpp>
#include <cadmium/io/iestream.hpp>


#include <cadmium/real_time/arm_mbed/io/digitalInput.hpp>
#include <cadmium/real_time/arm_mbed/io/digitalOutput.hpp>
#include "../atomics/lcd.hpp"
#include "../atomics/alarm_clock.hpp"

#ifdef RT_ARM_MBED
  #include "../mbed.h"
#else
  // When simulating the model it will use these files as IO in place of the pins specified.
  const char* BUTTON1 = "./inputs/BUTTON1_In.txt"; //Start & Stop the clock
  //const char* LED1    = "./outputs/LED1_Out.txt"; //
  const char* D15 = "./inputs/MINUTE_IN.txt";
  const char* D14 = "./inputs/HOUR_IN.txt";
#endif

using namespace std;

using hclock=chrono::high_resolution_clock;
using TIME = NDTime;


int main(int argc, char ** argv) {

  #ifdef RT_ARM_MBED
      //Logging is done over cout in RT_ARM_MBED
      struct oss_sink_provider{
        static std::ostream& sink(){
          return cout;
        }
      };
  #else
    // all simulation timing and I/O streams are ommited when running embedded
    auto start = hclock::now(); //to measure simulation execution time

    static std::ofstream out_data("alarm_clock_output.txt");
    struct oss_sink_provider{
      static std::ostream& sink(){
        return out_data;
      }
    };
  #endif

  /*************** Loggers *******************/
  using info=cadmium::logger::logger<cadmium::logger::logger_info, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
  using debug=cadmium::logger::logger<cadmium::logger::logger_debug, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
  using state=cadmium::logger::logger<cadmium::logger::logger_state, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
  using log_messages=cadmium::logger::logger<cadmium::logger::logger_messages, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
  using routing=cadmium::logger::logger<cadmium::logger::logger_message_routing, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
  using global_time=cadmium::logger::logger<cadmium::logger::logger_global_time, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
  using local_time=cadmium::logger::logger<cadmium::logger::logger_local_time, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
  using log_all=cadmium::logger::multilogger<info, debug, state, log_messages, routing, global_time, local_time>;

  using logger_top=cadmium::logger::multilogger<log_messages, global_time>;


  /*******************************************/

  using AtomicModelPtr=std::shared_ptr<cadmium::dynamic::modeling::model>;
  using CoupledModelPtr=std::shared_ptr<cadmium::dynamic::modeling::coupled<TIME>>;

  /********************************************/
  /***************** Alarm Clock **************/
  /********************************************/

  AtomicModelPtr alarmClock = cadmium::dynamic::translate::make_dynamic_atomic_model<alarm_clock, TIME>("alarmClock"); //The alarm clock atomic

  /********************************************/
  /********** Buttons *******************/
  /********************************************/
  AtomicModelPtr toggleClockButton = cadmium::dynamic::translate::make_dynamic_atomic_model<DigitalInput, TIME> ("toggleClockButton", BUTTON1);  //The Button Atomic

  AtomicModelPtr setHour = cadmium::dynamic::translate::make_dynamic_atomic_model<DigitalInput, TIME>("setHour", D15);  //The Button Atomic

  AtomicModelPtr setMin = cadmium::dynamic::translate::make_dynamic_atomic_model<DigitalInput, TIME>("setMin", D14);  //The Button Atomic

  /********************************************/
  /********** TM1637 Driver *******************/
  /********************************************/
  AtomicModelPtr lcdOutput = cadmium::dynamic::translate::make_dynamic_atomic_model<LCD, TIME>("lcdOutput"); //The TM1637 Driver Atomic



  /************************/
  /*******TOP MODEL********/
  /************************/
  cadmium::dynamic::modeling::Ports iports_TOP = {};
  cadmium::dynamic::modeling::Ports oports_TOP = {};
  cadmium::dynamic::modeling::Models submodels_TOP =  {alarmClock, toggleClockButton, lcdOutput, setHour, setMin};
  cadmium::dynamic::modeling::EICs eics_TOP = {};
  cadmium::dynamic::modeling::EOCs eocs_TOP = {};
  cadmium::dynamic::modeling::ICs ics_TOP = {
    //cadmium::dynamic::translate::make_IC<_defs::dataOut, digitalOutput_defs::in>("alarmClock","digitalOutput1"),

    cadmium::dynamic::translate::make_IC<digitalInput_defs::out, alarm_clock_defs::in>("toggleClockButton", "alarmClock"),
    cadmium::dynamic::translate::make_IC<digitalInput_defs::out, alarm_clock_defs::setHour>("setHour", "alarmClock"),
    cadmium::dynamic::translate::make_IC<digitalInput_defs::out, alarm_clock_defs::setMin>("setMin", "alarmClock"),

    cadmium::dynamic::translate::make_IC<alarm_clock_defs::out, LCD_defs::in>("alarmClock", "lcdOutput")

  };
  CoupledModelPtr TOP = std::make_shared<cadmium::dynamic::modeling::coupled<TIME>>(
    "TOP",
    submodels_TOP,
    iports_TOP,
    oports_TOP,
    eics_TOP,
    eocs_TOP,
    ics_TOP
  );

  ///****************////
  cadmium::dynamic::engine::runner<NDTime, logger_top> r(TOP, {0});
  r.run_until(NDTime("10:00:00:000"));
  #ifndef RT_ARM_MBED
    return 0;
  #endif
}
