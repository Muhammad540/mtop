#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <string>
#include <vector>

#include "process.hpp"
#include "processor.hpp"

class System {
  public:
    Processor& Cpu();
    std::vector<Process>& Processes();
    float MemoryUtilization();
    long UpTime();
    int TotalProcesses();
    int RunningProcesses();
    std::string Kernel();
    std::string OperatingSystem();

  private:
    Processor cpu_ = {};
    std::vector<Process> processes_ = {};
};

#endif