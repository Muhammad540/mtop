#ifndef SYSTEM_PARSER_HPP
#define SYSTEM_PARSER_HPP

#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <dirent.h>

namespace LinuxParser {
    const std::string kProcDirectory{"/proc/"};
    const std::string kCmdlineFilename{"/cmdline"};
    const std::string kCpuinfoFilename{"/cpuinfo"};
    const std::string kStatusFilename{"/status"};
    const std::string kStatFilename{"/stat"};
    const std::string kUptimeFilename{"/uptime"};
    const std::string kMeminfoFilename{"/meminfo"};
    const std::string kVersionFilename{"/version"};
    const std::string kOSPath{"/etc/os-release"};
    const std::string kPasswordPath{"/etc/passwd"};

    float MemoryUtilization();
    long UpTime();
    std::vector<int> Pids();
    int TotalProcesses();
    int RunningProcesses();
    std::string OperatingSystem();
    std::string Kernel();

    /*  brief descruption of /proc/stats 
        user	time spent executing normal user space processes
	    nice	time spent on "niced" (lower priority) user space processes
	    system	time spent executing kernel space code (e.g system calls)
	    idle	time the CPU was completely idle
	    iowait	time spent waiting for I/O
	    irq	    time servicing hardware interrupts
	    softirq	time servicing software interrupts
	    steal	time stolen by the hypervisor for other tasks
    */
    enum CPUStates {
        kUser_ = 0,
        kNice_,
        kSystem_,
        kIdle_,
        kIOwait_,
        kIRQ_,
        kSoftIRQ_,
        kSteal_,
        kGuest_,
        kGuestNice_
    };

    struct CpuTimes {
        long user{0}, nice{0}, system{0}, idle{0}, iowait{0}, irq{0}, softirq{0}, steal{0};
    };

    std::vector<std::string> CpuUtilization();
    bool ReadCpuTimesAll(std::vector<CpuTimes>& out);
    float UtilFromData(const CpuTimes& prev, const CpuTimes& curr);
    long Jiffies();
    long ActiveJiffies();
    long ActiveJiffies(int pid);
    long IdleJiffies();

    std::string Command(int pid);
    std::string Ram(int pid);
    std::string Uid(int pid);
    std::string User(int pid);
    long int UpTime(int pid);
}; // namespace LinuxParser

#endif