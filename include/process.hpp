#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <string>

class Process {
    public:
        explicit Process(int pid);
        int Pid();
        std::string User();
        std::string Command();
        float CpuUtilization();
        std::string Ram();
        long int UpTime();
        bool operator<(Process const& a) const;
    
    private:
        int pid_{0};        
};

#endif