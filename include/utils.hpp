#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>
#include <iomanip>

namespace Utils {
    //  input:  Long int measuring seconds 
    // output:  HH::MM::SS 
    std::string ElapsedTime(long times); 
};

#endif 