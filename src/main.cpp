#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <ftxui/screen/color.hpp>
#include <iostream>

int main() {
    using namespace ftxui;

    ScreenInteractive screen = ScreenInteractive::Fullscreen();

    auto ui = Renderer([&]{
        
        auto header = hbox({
            text("mtop") | bold, 
            filler(), 
            text("Uptime: "),
            text(" "),
            text("q: quit") | dim,
        }) | bgcolor(Color::Black);

        auto cpu = vbox({
            text("CPU") | bold,
            gauge(0.5) | color(Color::Green) | border,
        }) | flex;

        auto mem = vbox({
            text("Memory (non-cache/buffers)") | bold,
            gauge(0.5) | color(Color::Yellow) | border,
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

        return vbox({
            header,
            separator(),
            hbox({ cpu, separator(), mem}),
            separator(),
        }) | border;
    
    });
    
    auto ui_with_keys = CatchEvent(ui, [&](Event e){
        if (e == Event::Character('q') || e == Event::Character('Q')) {
            screen.Exit();
            return true;
        }
        return false;
    });

    // Blocking until the component exits 
    screen.Loop(ui_with_keys);
    return 0;
}