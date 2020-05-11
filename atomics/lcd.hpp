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



#ifdef RT_ARM_MBED

#include "../mbed-os/mbed.h"
#include "../TM1637_DRIVER/TM1637.h"

#if (CATALEX_TEST == 1)
// CATALEX TM1637 4 Digit display test
#include "../TM1637_DRIVER/Font_7Seg.h"
#endif
//TM1637_CATALEX declaration
#if(SPI==1)
//Old Constructor
//TM1637_CATALEX CATALEX(p5, p6, p7);   //LPC1768
TM1637_CATALEX CATALEX(D8, D9, D10);  //F401
#else
//New Constructor
//TM1637_CATALEX CATALEX(p6, p7);       //LPC1768
TM1637_CATALEX CATALEX(D9, D10);      //F401     //I exclusivly use this one
#endif

/******************************************************************************
* REAL-TIME IMPLEMENTATION
*****************************************************************************/

using namespace cadmium;
using namespace std;

//Port definition
struct LCD_defs{
    struct in : public in_port<string> {};
};

template <typename TIME> class LCD {
    using defs=LCD_defs; // putting definitions in context

public:
    // default c onstructor

    char* str_char;

    LCD() noexcept{
        state.output = "8888";
        CATALEX.setBrightness(TM1637_BRT2); //brightness
        //CATALEX.setIcon(TM1637_CATALEX::COL2); //the colon in the middle of the two digits
    }

    // state definition
    struct state_type{
        string output;
    };
    state_type state;

    // ports definition
    using input_ports=std::tuple<typename defs::in>;
    using output_ports=std::tuple<>;

    // internal transition
    void internal_transition() {}

    // external transition
    void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        for(const auto &x : get_messages<typename defs::in>(mbs)){
            state.output = x;
        }

        CATALEX.locate(0); //set cursor to the first digit
        str_char = &*state.output.begin(); //convert the string datatype to a char* datatype

        CATALEX.printf(str_char); //print to the display
        CATALEX.setIcon(TM1637_CATALEX::COL2); //display the colon
    }
    // confluence transition
    void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        internal_transition();
        external_transition(TIME(), std::move(mbs));
    }

    // output function
    typename make_message_bags<output_ports>::type output() const {
        typename make_message_bags<output_ports>::type bags;

        return bags;
    }

    // time_advance function
    TIME time_advance() const {
        return std::numeric_limits<TIME>::infinity();
    }

    friend std::ostringstream& operator<<(std::ostringstream& os, const typename LCD<TIME>::state_type& i) {
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
const char* LCD_FILE = "./outputs/LCD_out.txt";

//Port definition
struct LCD_defs{
    struct in : public in_port<string> {};
};

template<typename TIME>
class LCD : public oestream_output<string, TIME, LCD_defs>{
public:
    LCD() : oestream_output<string, TIME, LCD_defs>(LCD_FILE) {}
};

#endif //RT_ARM_MBED
