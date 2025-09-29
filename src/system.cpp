#include "../include/system.hpp"
#include "../include/linux_parser.hpp"

Processor& System::Cpu() { return cpu_; }

std::vector<Process>& System::Processes() {
    processes_.clear();
    for (int pid : LinuxParser::Pids()) {
        processes_.emplace_back(pid);
    }
    std::sort(processes_.begin(), processes_.end(), [](const Process& a, const Process& b){
        return b < a;
    });
    return processes_;
}

float System::MemoryUtilization() { return LinuxParser::MemoryUtilization(); }
long System::UpTime() { return LinuxParser::UpTime(); }
int System::TotalProcesses() { return LinuxParser::TotalProcesses(); }
int System::RunningProcesses() { return LinuxParser::RunningProcesses(); }
std::string System::Kernel() { return LinuxParser::Kernel(); }
std::string System::OperatingSystem() { return LinuxParser::OperatingSystem(); }