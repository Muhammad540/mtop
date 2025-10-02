#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp> 

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/dom/linear_gradient.hpp> 
#include "../include/utils.hpp"
#include "../include/system.hpp"
#include "../include/linux_parser.hpp"

#include <array>
#include <mutex>
#include <string>
#include <iostream>
#include <deque>
#include <vector>
#include <algorithm>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <functional>

struct AppState {
    float total_cpu{0.f};
    float mem_used{0.f};
    long uptime{0};
    std::vector<std::array<std::string, 5>> procs;

    std::deque<float> cpu_history;
    std::deque<float> mem_history;
    std::vector<std::deque<float>> per_core_history;
};

int main() {
    using namespace ftxui;

    ScreenInteractive screen = ScreenInteractive::Fullscreen();
    System sys;

    std::mutex mtx;
    std::atomic<bool> running{true};
    std::atomic<bool> show_cores{false};
    AppState state;

    const int kMaxPoints = 160;
    std::vector<LinuxParser::CpuTimes> prev_times;

    auto push_hist = [&](std::deque<float>& dq, float val) {
        dq.push_back(val);
        if ((int)dq.size() > kMaxPoints) dq.pop_front();
    };

    auto refresh = [&] {
        AppState s;
        s.mem_used = sys.MemoryUtilization();
        s.uptime   = sys.UpTime();
    
        std::vector<LinuxParser::CpuTimes> curr_times;
        if (LinuxParser::ReadCpuTimesAll(curr_times)) {
            if (prev_times.empty()) {
                prev_times = curr_times;
            }
            const size_t overlap = std::min(prev_times.size(), curr_times.size());
            if (overlap >= 1) {
                s.total_cpu = LinuxParser::UtilFromData(prev_times[0], curr_times[0]);
                std::vector<float> per_core_now;
            
                per_core_now.reserve(overlap - 1);
                for (size_t i = 0; i < overlap - 1; ++i) {
                    per_core_now.push_back(LinuxParser::UtilFromData(prev_times[i + 1], curr_times[i + 1]));
                }
            
                {
                    std::lock_guard<std::mutex> lk(mtx);
    
                    push_hist(state.cpu_history, s.total_cpu);
                    push_hist(state.mem_history, s.mem_used);
    
                    if (state.per_core_history.size() != per_core_now.size()) {
                        state.per_core_history.assign(per_core_now.size(), {});
                    }
                    for (size_t i = 0; i < per_core_now.size(); ++i) {
                        push_hist(state.per_core_history[i], per_core_now[i]);
                    }
    
                    state.total_cpu = s.total_cpu;
                    state.mem_used  = s.mem_used;
                    state.uptime    = s.uptime;
                }
            }
            prev_times = std::move(curr_times);
        }
    
        auto& processes = sys.Processes();
        std::sort(processes.begin(), processes.end(),
                  [](const Process& a, const Process& b) { return b < a; });
    
        int count = 0;
        for (auto& p : processes) {
            std::string cmd = p.Command();
            if (cmd.empty()) continue;
            const int pid = p.Pid();
            const std::string user = p.User();
            const float cpu = p.CpuUtilization() * 100.f; // TODO: delta-based per-process
            const std::string mem = p.Ram();
            if (cmd.size() > 40) cmd = cmd.substr(0, 37) + "...";
    
            s.procs.push_back({
                std::to_string(pid),
                user.empty() ? std::string("?") : user,
                std::to_string(static_cast<int>(cpu)),
                mem,
                cmd
            });
            if (count++ >= 15) break;
        }
    
        {
            std::lock_guard<std::mutex> lk(mtx);
            state.procs = std::move(s.procs);
        }
        screen.Post(Event::Custom);
    };

    std::thread sampler([&]{
        ReadCpuTimesAll(prev_times);
        while (running.load()) {
            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    auto graph_from = [&](const std::deque<float>& hist) {
        return [&](int width, int height) {
            std::vector<int> out(width, 0);
            if (hist.empty() || width <= 0 || height <= 0) return out;
            int n = (int)hist.size();
            for (int x = 0; x < width; ++x) {
                int idx = std::max(0, n - width + x);
                float v = hist[idx];
                int h = std::clamp((int)std::round(v * height), 0, height);
                out[x] = h;
            }
            return out;
        };
    };

    auto ui = Renderer([&]{
        std::lock_guard<std::mutex> lk(mtx);
        auto cpu_graphfn = graph_from(state.cpu_history);
        auto mem_graphfn = graph_from(state.mem_history);
        auto header = hbox({
            text("mtop") | bold, 
            filler(), 
            text("Uptime: " + Utils::ElapsedTime(state.uptime)),
            text("  "),
            text("c: cores  q: quit") | dim,
        }) | bgcolor(Color::Black);

        auto cpu_graph = vbox({
            text("CPU Utilization [%]") | bold,
            hbox({
                vbox({
                    text("1.00"),
                    filler(),
                    text("0.75"),
                    filler(),
                    text("0.50"),
                    filler(),
                    text("0.25"),
                    filler(),
                    text("0.00"),
                }),
                graph(cpu_graphfn) | color(Color::Green) | flex,
            }) | flex,
        }) | borderRounded;

        auto mem_graph = vbox({
            text("Memory Used (non cache/buffers)") | bold,
            hbox({
                vbox({
                    text("100 "),
                    filler(),
                    text("50"),
                    filler(),
                    text("0"),
                }),
                graph(mem_graphfn) | color(Color::Yellow) | flex,
            }) | flex,
        }) | borderRounded;

        Elements core_rows;
        if (show_cores.load() && !state.per_core_history.empty()) {
            int cols = 2;
            int i = 0;
            while (i < (int)state.per_core_history.size()) {
                Elements row;
                for (int c = 0; c < cols && i < (int)state.per_core_history.size(); ++c, ++i) {
                    auto label = "cpu" + std::to_string(i);
                    row.push_back(vbox({
                        text(label),
                        graph(graph_from(state.per_core_history[i])) | color(Color::Green) | flex,
                    }) | border | flex);
                }
                core_rows.push_back(hbox(std::move(row)) | flex);
            }
        }

        std::vector<Element> rows;
        rows.push_back(
            hbox({
                text("PID") | bold | size(WIDTH, EQUAL, 8),
                text("USER") | bold | size(WIDTH, EQUAL, 10),
                text("CPU%") | bold | size(WIDTH, EQUAL, 6),
                text("MEM(MB)") | bold | size(WIDTH, EQUAL, 10),
                text("COMMAND") | bold | flex,
            }) | bgcolor(Color::DarkBlue)
        );

        for (auto& r: state.procs) {
            rows.push_back(
                hbox({
                    text(r[0]) | size(WIDTH, EQUAL, 8),
                    text(r[1]) | size(WIDTH, EQUAL, 8),
                    text(r[2]) | size(WIDTH, EQUAL, 8),
                    text(r[3]) | size(WIDTH, EQUAL, 8),
                    text(r[4]) | flex,
                })
            );
        }

        auto table = vbox(std::move(rows)) | border;
        
        auto background = bgcolor(LinearGradient()
                    .Angle(100.f)
                    .Stop(Color::Blue, 0.f)
                    .Stop(Color::Red, 1.f));

        auto foreground = vbox({
            header,
            separator(),
            hbox({ cpu_graph | flex, separator(), mem_graph | flex }),
            separator(),
            (show_cores.load() ? text("Per-core: expanded (press 'c' to collapse)") : text("Per-core: collapsed (press 'c' to expand)")) | dim,
            vbox(std::move(core_rows)) | flex,
            separator(),
            table | flex,
        }) | flex ;

        return dbox(background, foreground);
    });
    
    auto ui_with_keys = CatchEvent(ui, [&](Event e){
        if (e == Event::Character('q') || e == Event::Character('Q')) {
            running = false;
            screen.Exit();
            return true;
        }
        if (e == Event::Character('c') || e == Event::Character('C')) {
            show_cores = !show_cores.load();
            screen.Post(Event::Custom);
            return true;
        }
        return false;
    });

    // Blocking until the component exits 
    screen.Loop(ui_with_keys);
    running = false;
    sampler.join();
    return 0;
}