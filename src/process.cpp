#include "../include/process.hpp"
#include "../include/linux_parser.hpp"
#include <unistd.h>

Process::Process(int pid): pid_(pid) {}
int Process::Pid() { return pid_; }
std::string Process::User() { return LinuxParser::User(pid_); }
std::string Process::Command() { return LinuxParser::Command(pid_); }

float Process::CpuUtilization() {
    long total_time = LinuxParser::ActiveJiffies(pid_);
    long hertz = sysconf(_SC_CLK_TCK);
    long seconds = LinuxParser::UpTime() - (LinuxParser::UpTime(pid_));
    if (seconds <= 0) return 0.f;
    float cpu = (static_cast<float>(total_time) / static_cast<float>(hertz)) / static_cast<float>(seconds);
    return cpu;
}

std::string Process::Ram() { return LinuxParser::Ram(pid_); }

long int Process::UpTime() { return LinuxParser::UpTime(pid_); }

bool Process::operator<(Process const& a) const {
    float lhs = const_cast<Process*>(this)->CpuUtilization();
    float rhs = const_cast<Process&>(a).CpuUtilization();
    return lhs < rhs;
}