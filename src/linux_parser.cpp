#include "../include/linux_parser.hpp"
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <ios>
#include <limits>
#include <sstream>
#include <string>
#include <unistd.h>

namespace {
    bool ReadFile(const std::string& path, std::string& out) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        std::ostringstream ss;
        ss << file.rdbuf();
        out = ss.str();
        return true;
    }

    bool GetValueByKey(const std::string& path, const std::string& key, std::string& value) {
        // to read files with KEY="VALUE" mapping like /etc/os-release
        std::ifstream stream(path);
        if (!stream.is_open()) return false;
        
        std::string line;

        while (std::getline(stream, line)) {
            size_t equal_sign_pos = line.find('=');
            if (equal_sign_pos == std::string::npos) continue;
            std::string current_key = line.substr(0, equal_sign_pos);
            if (current_key == key) {
                value = line.substr(equal_sign_pos+1);
                value.erase(std::remove(value.begin(), value.end(),'\"'), value.end());
                return true;
            }
        }
        return false;
    }

    long ToLong(const std::string& s) {
        try {
            return std::stol(s);
        } catch(...) {
            return 0;
        }
    }
}; // namespace 

float LinuxParser::MemoryUtilization() {
    long mem_total = 0, mem_free = 0, buffers = 0, cached = 0, sreclaimable = 0, shmem = 0;

    std::ifstream file(kMeminfoFilename);
    if (!file.is_open()) return 0;
    std::string key;
    long value;
    std::string unit;
    while (file >> key >> value >> unit) {
        if (key == "MemTotal:") mem_total = value;
        else if (key == "MemFree:") mem_free = value;
        else if (key == "Buffers:") buffers = value;
        else if (key == "Cached:") cached = value;
        else if (key == "SReclaimable:") sreclaimable = value;
        else if (key == "Shmem:") shmem = value;
    }

    if (mem_total == 0) return 0;
    long used = mem_total - mem_free;
    long cached_all = cached + sreclaimable - shmem;
    long non_cache_used = used - (buffers +  cached_all);
    if (non_cache_used < 0) non_cache_used = 0;
    return static_cast<float>(non_cache_used) / static_cast<float>(mem_total);
}

long LinuxParser::UpTime() {
    std::ifstream file(kUptimeFilename);
    double uptime{0.0};
    file >> uptime;
    return static_cast<long>(uptime);
}

std::vector<int> LinuxParser::Pids() {
    std::vector<int> pids;
    DIR* directory = opendir(kProcDirectory.c_str());
    if (!directory) return pids;
    struct dirent* file;
    while ((file = readdir(directory)) != nullptr) {
        if (file->d_type == DT_DIR) {
            std::string filename(file->d_name);
            if (!filename.empty() && std::all_of(filename.begin(), filename.end(), ::isdigit)) {
                pids.push_back(std::stoi(filename));
            }
        }
    }
    closedir(directory);
    return pids;
}

int LinuxParser::TotalProcesses() {
    std::ifstream file(kStatFilename);
    if (!file.is_open()) return 0;
    std::string key;
    int value;
    while (file >> key >> value) {
        if (key == "processes") return value;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return 0;
}

int LinuxParser::RunningProcesses() {
    std::ifstream file(kStatFilename);
    if (!file.is_open()) return 0;
    std::string key;
    int value;
    while (file >> key >> value) {
        if (key == "procs_running") return value;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return 0;
}

std::string LinuxParser::OperatingSystem() {
    std::ifstream file(kOSPath);
    if (!file.is_open()) return {};
    std::string line, key, value;
    while (std::getline(file, line)) {
        std::replace(line.begin(), line.end(), ' ', '_');
        std::replace(line.begin(), line.end(), '=', ' ');
        std::replace(line.begin(), line.end(), '"', ' ');
        std::istringstream linestream(line);
        while (linestream >> key >> value) {
            if (key == "PRETTY_NAME") {
                std::replace(value.begin(), value.end(), '_',' ');
                return value;
            }
        }
    }
    return {};
}

std::string LinuxParser::Kernel() {
    std::ifstream stream(kVersionFilename);
    if (!stream.is_open()) return {};
    std::string os, version, kernel;
    std::getline(stream, os, ' ');
    std::getline(stream, version, ' ');
    std::getline(stream, kernel, ' ');
    return kernel;
}

std::vector<std::string> LinuxParser::CpuUtilization() {
    /*
        user       - Normal processes in user mode
        nice       - Niced processes in user mode
        system     - Kernel mode
        idle       - Idle time
        iowait     - Waiting for I/O
        irq        - Hardware interrupts
        softirq    - Software interrupts
        steal      - Involuntary wait
        guest      - Running guest VMs
        guest_nice - Niced guest VMs
    */
    std::ifstream file(kStatFilename);
    if (!file.is_open()) return {};
    std::string line;
    std::getline(file, line);
    std::istringstream ss(line);
    std::string cpu;
    ss >> cpu;
    std::vector<std::string> times;
    std::string val;
    while (ss >> val) times.push_back(val);
    return times;
}

long LinuxParser::Jiffies() {
    auto v = CpuUtilization();
    long sum = 0;
    for (auto& s : v) {
        sum += ToLong(s);
    }
    return sum;
}

long LinuxParser::ActiveJiffies() {
    auto v = CpuUtilization();
    if (v.size() < 10) return 0;
    long user = ToLong(v[kUser_]);
    long nice = ToLong(v[kNice_]);
    long system = ToLong(v[kSystem_]);
    long irq = ToLong(v[kIRQ_]);
    long softirq = ToLong(v[kSoftIRQ_]);
    long steal = ToLong(v[kSteal_]);
    return user + nice + system + irq + softirq + steal;
}

long LinuxParser::IdleJiffies() {
    auto v = CpuUtilization();
    if (v.size() < 5) return 0;
    long idle = ToLong(v[kIdle_]);
    long iowait = ToLong(v[kIOwait_]);
    return idle + iowait;
}

long LinuxParser::ActiveJiffies(int pid) {
    // proc/[pid]/stat
    /*
        pid: The unique ID of the process.
        comm: The command name of the executable (in parentheses).
        state: A single character representing the process state (S=Sleeping, R=Running, Z=Zombie)
        ppid: The process ID of the parent process.
        pgrp: The process group ID of the process.
        session: The session ID of the process.
        tty_nr: The controlling terminal of the process.
        tpgid: The ID of the foreground process group of the controlling terminal.
        flags: The kernel flags for the process
        minflt: The number of minor page faults
        cminflt: The number of minor page faults made by the process and its "waitedfor" children.
        majflt: The number of major page faults
        cmajflt: The number of major page faults made by the process and its "waitedfor" children.
        --- we track the following (clock ticks)---
        utime: Time the process has spent in user mode 
        stime: Time the process has spent in kernel mode 
        cutime: Time the process's "waitedfor" children have spent in user mode
        cstime: Time the process's "waitedfor" children have spent in kernel mode
        priority: The priority of the process.
        nice: The nice value of the process, which influences its priority.
        num_threads: The number of threads in this process.
        itrealvalue: Time in jiffies before the next SIGALRM is sent to the process.
        starttime: The time the process started after system boot 
        vsize: The virtual memory size of the process in bytes.
        rss: Resident Set Size: The number of pages the process has in physical memory.
    */ 
    std::string content;
    if (!ReadFile(kProcDirectory + std::to_string(pid) + "/stat", content)) return 0;
    std::istringstream ss(content);
    std::string token;
    int idx = 0;
    long utime = 0, stime=0, cutime=0, cstime=0;
    while (ss >> token) {
        ++idx;
        if (idx == 14) utime = ToLong(token);
        else if (idx == 15) stime = ToLong(token);
        else if (idx == 16) cutime = ToLong(token);
        else if (idx == 17) {cstime = ToLong(token); break; }
    }
    return utime + stime + cutime + cstime;
}

std::string LinuxParser::Command(int pid) {
    std::string content;
    if (!ReadFile(kProcDirectory + std::to_string(pid) + kCmdlineFilename, content)) return {};
    std::replace(content.begin(), content.end(), '\0', ' ');
    while (!content.empty() && std::isspace(static_cast<unsigned char>(content.back()))) content.pop_back();
    return content;
}

std::string LinuxParser::Ram(int pid) {
    // Return MB as a string 
    std::ifstream file(kProcDirectory + std::to_string(pid) + "/status");
    if (!file.is_open()) return "0";
    std::string key;
    long value = 0;
    std::string unit;
    while (file >> key >> value >> unit) {
        if (key == "VmRSS:") {
            long mb = value / 1024;
            return std::to_string(mb);
        }
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return "0";
}

std::string LinuxParser::Uid(int pid) {
    std::ifstream file(kProcDirectory + std::to_string(pid)+"/status");
    if (!file.is_open()) return {};
    std::string key, value;
    while (file >> key >> value) {
        if (key == "Uid:") return value;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return {};
}

std::string LinuxParser::User(int pid) {
    std::string uid = Uid(pid);
    if (uid.empty()) return {};
    std::ifstream file(kPasswordPath);
    if (!file.is_open()) return {};
    std::string line;
    while (std::getline(file, line)) {
        std::replace(line.begin(), line.end(), ':', ' ');
        std::istringstream ss(line);
        std::string user, x, id;
        if (ss >> user >> x >> id) {
            if (id == uid) return user;
        }
    }
    return {};
}

long int LinuxParser::UpTime(int pid) {
    // Process uptime = system uptime - start time/HZ
    std::string content;
    if (!ReadFile(kProcDirectory + std::to_string(pid) + "/stat",content)) return 0;
    std::istringstream ss(content);
    std::string token;
    int idx = 0;
    long starttime = 0;
    while (ss >> token) {
        ++idx;
        if (idx == 22) {
            starttime = ToLong(token);
            break;
        }
    }
    long hertz = sysconf(_SC_CLK_TCK);
    long uptime = UpTime();
    long seconds = uptime - (starttime / hertz);
    if (seconds < 0) seconds = 0;
    return seconds;
}