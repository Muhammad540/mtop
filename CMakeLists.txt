cmake_minimum_required(VERSION 3.22)
project(mtop)

include(FetchContent)
 
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v6.1.9  # Replace with a version, tag, or commit hash
)
 
FetchContent_MakeAvailable(ftxui)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(mtop ${SOURCES})

set_property(TARGET mtop PROPERTY CXX_STANDARD_17)
target_link_libraries(mtop
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)
target_compile_options(mtop PRIVATE -Wall -Wextra)