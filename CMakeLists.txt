cmake_minimum_required(VERSION 4.0)
project(OSUCourseTree)

set(CMAKE_CXX_STANDARD 26)

find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

file(GLOB src
        "src/*.h"
        "src/*.cpp"
        "imgui/*.h"
        "imgui/*.cpp"
        "imgui/backends/*.cpp"
        "imgui/backends/*.h"
        "imgui/misc/*.cpp"
        "imgui/misc/*.h"
)

add_executable(OSUCourseTree ${src})

target_link_libraries(OSUCourseTree PUBLIC glad::glad)
target_link_libraries(OSUCourseTree PUBLIC glm::glm)
target_link_libraries(OSUCourseTree PUBLIC SDL3::SDL3)

add_custom_command(TARGET OSUCourseTree PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:OSUCourseTree>/assets
)

target_include_directories(OSUCourseTree PRIVATE
        imgui
        imgui/backends
)