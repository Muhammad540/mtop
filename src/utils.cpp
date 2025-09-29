#include "../include/utils.hpp"

std::string Utils::ElapsedTime(long times) {
    long h = times / 3600;
    long m = (times % 3600) / 60;
    long s = times % 60;
    std::ostringstream oss;

    oss << std::setw(2) << std::setfill('0') << h << ":"
        << std::setw(2) << std::setfill('0') << m << ":"
        << std::setw(2) << std::setfill('0') << s;
    return oss.str();
}