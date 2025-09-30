#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <ftxui/screen/color.hpp>
#include "../include/utils.hpp"
#include "../include/system.hpp"
#include <array>
#include <mutex>
#include <string>
#include <iostream>
struct AppState {
    float total_cpu{0.f};
    float mem_used{0.f};
    long uptime{0};
    std::vector<std::array<std::string, 5>> procs;
};

int main() {
    using namespace ftxui;

    ScreenInteractive screen = ScreenInteractive::Fullscreen();
    System sys;
    std::mutex mtx;
    std::atomic<bool> running{true};
    AppState state;

    auto refresh = [&] {
        AppState s;
        s.total_cpu = sys.Cpu().Utilization();
        s.mem_used = sys.MemoryUtilization();
        s.uptime = sys.UpTime();

        auto& processes = sys.Processes();
        std::sort(processes.begin(), processes.end(), [](const Process&a, const Process& b){
            return b < a;
        });

        int count = 0;
        for (auto& p: processes) {
            const int pid = p.Pid();
            const std::string user = p.User();
            const float cpu = p.CpuUtilization() * 100.f;
            const std::string mem = p.Ram();
            std::string cmd = p.Command();
            if (cmd.empty()) continue;
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
            state = std::move(s);
        }
        screen.Post(Event::Custom);
    };

    std::thread sampler([&]{
        while (running.load()) {
            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    auto ui = Renderer([&]{
        std::lock_guard<std::mutex> lk(mtx);
        
        auto header = hbox({
            text("mtop") | bold, 
            filler(), 
            text("Uptime: " + Utils::ElapsedTime(state.uptime)),
            text(" "),
            text("q: quit") | dim,
        }) | bgcolor(Color::Black);

        auto cpu = vbox({
            text("CPU") | bold,
            gauge(state.total_cpu) | color(Color::Green) | border,
        }) | flex;

        auto mem = vbox({
            text("Memory (non cache/buffers)") | bold,
            gauge(state.mem_used) | color(Color::Yellow) | border,
        }) | flex;

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

        return vbox({
            header,
            separator(),
            hbox({ cpu, separator(), mem}),
            separator(),
            table | flex,
        }) | border;
    
    });
    
    auto ui_with_keys = CatchEvent(ui, [&](Event e){
        if (e == Event::Character('q') || e == Event::Character('Q')) {
            running = false;
            screen.Exit();
            return true;
        }
        return false;
    });

    // Blocking until the component exits 
    screen.Loop(ui_with_keys);
    sampler.join();
    return 0;
}