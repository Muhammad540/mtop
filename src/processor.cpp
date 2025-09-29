#include "../include/processor.hpp"
#include "../include/linux_parser.hpp"
#include <string>

float Processor::Utilization() {
    auto v = LinuxParser::CpuUtilization();
    if (v.size() < 5) return 0.f;

    long user = std::stol(v[LinuxParser::kUser_]);
    long nice = std::stol(v[LinuxParser::kNice_]);
    long system = std::stol(v[LinuxParser::kSystem_]);
    long idle = std::stol(v[LinuxParser::kIdle_]);
    long iowait = std::stol(v[LinuxParser::kIOwait_]);
    long irq = std::stol(v[LinuxParser::kIRQ_]);
    long softirq = std::stol(v[LinuxParser::kSoftIRQ_]);
    long steal = std::stol(v[LinuxParser::kSteal_]);
    
    long idle_all = idle + iowait;
    long non_idle = user + nice + system + irq + softirq + steal;
    long total = idle_all + non_idle;

    if (prev_total_ == 0 && prev_idle_ == 0) {
        prev_total_ = total;
        prev_idle_ = idle_all;
        return 0.f;
    }

    long totald = total - prev_total_;
    long idled = idle_all - prev_idle_;
    prev_total_ = total;
    prev_idle_ = idle_all;

    if (totald <= 0) return 0.f;
    return static_cast<float>(totald - idled) / static_cast<float>(totald);
}