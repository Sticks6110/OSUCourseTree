#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include <misc/imgui_stdlib.h>

#include "courses.h"
#include "graph_window.h"

namespace {

std::string join(const std::vector<std::string>& values) {
    std::string result;
    for (const std::string& value : values) {
        if (!result.empty()) result += ", ";
        result += value;
    }
    return result;
}

std::string catalog_subject_url(const std::string& subject) {
    std::string result;
    result.reserve(subject.size());
    for (const unsigned char character : subject) {
        if (std::isalnum(character)) result.push_back(static_cast<char>(std::tolower(character)));
    }
    return result;
}

void show_text(const std::string& text) {
    ImGui::TextWrapped("%s", text.empty() ? "Not listed." : text.c_str());
}

} // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("OSU Course Tree", 1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr || !SDL_GL_MakeCurrent(window, gl_context)) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << '\n';
        if (gl_context != nullptr) SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        std::cerr << "Unable to load OpenGL functions. OpenGL 4.6 support is required.\n";
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "assets/layout.ini";
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    int result = 0;
    try {
        courses catalog("assets/osu_courses_2026_2027_processed.json");
        graph_window main_graph("Course graph###main", catalog.generate_graph(), &catalog);
        std::vector<std::unique_ptr<graph_window>> course_graphs;
        std::string search_text;
        Course selected_course;
        unsigned int graph_id = 0;

        const auto set_selected_course = [&](const Course& course) {
            if (!course.course_code.empty()) selected_course = course;
        };
        const auto open_course_graph = [&] {
            if (selected_course.course_code.empty()) return;
            ++graph_id;
            course_graphs.emplace_back(std::make_unique<graph_window>(
                selected_course.course_code + " prerequisites###course-" + std::to_string(graph_id),
                catalog.generate_course_graph(selected_course.course_code), &catalog));
        };

        float repulsion = 30.0f;
        float spring_strength = 0.05f;
        float spring_length = 10.0f;
        float spring_damping = 0.9f;
        float centering_strength = 0.01f;
        bool physics_dirty = true;
        bool running = true;
        Uint64 last_frame = SDL_GetPerformanceCounter();
        const Uint64 frequency = SDL_GetPerformanceFrequency();

        while (running) {
            const Uint64 now = SDL_GetPerformanceCounter();
            const float delta_time = std::clamp(static_cast<float>(now - last_frame) / static_cast<float>(frequency), 0.0f, 0.1f);
            last_frame = now;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                    continue;
                }
                main_graph.process_event(event);
                for (const auto& graph : course_graphs) graph->process_event(event);
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                    set_selected_course(main_graph.select_node_at_location(event.button.x, event.button.y));
                    for (const auto& graph : course_graphs) set_selected_course(graph->select_node_at_location(event.button.x, event.button.y));
                }
            }

            if (physics_dirty) {
                main_graph.set_physics_settings(repulsion, spring_strength, spring_length, spring_damping, centering_strength);
                for (const auto& graph : course_graphs) graph->set_physics_settings(repulsion, spring_strength, spring_length, spring_damping, centering_strength);
                physics_dirty = false;
            }

            int display_width = 0;
            int display_height = 0;
            SDL_GetWindowSizeInPixels(window, &display_width, &display_height);
            glViewport(0, 0, display_width, display_height);
            glClearColor(0.025f, 0.035f, 0.055f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("View")) {
                    if (ImGui::MenuItem("Save layout")) ImGui::SaveIniSettingsToDisk("assets/layout.ini");
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            main_graph.update(delta_time);
            for (const auto& graph : course_graphs) graph->update(delta_time);
            course_graphs.erase(std::remove_if(course_graphs.begin(), course_graphs.end(),
                                               [](const std::unique_ptr<graph_window>& graph) { return !graph->is_open(); }),
                                course_graphs.end());

            ImGui::Begin("Physics");
            physics_dirty |= ImGui::SliderFloat("Repulsion", &repulsion, 0.0f, 200.0f, "%.1f");
            physics_dirty |= ImGui::SliderFloat("Spring strength", &spring_strength, 0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
            physics_dirty |= ImGui::SliderFloat("Spring length", &spring_length, 1.0f, 100.0f, "%.1f");
            physics_dirty |= ImGui::SliderFloat("Damping", &spring_damping, 0.01f, 0.999f, "%.3f");
            physics_dirty |= ImGui::SliderFloat("Centering", &centering_strength, 0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);
            ImGui::End();

            ImGui::Begin("Find course");
            const bool submitted = ImGui::InputTextWithHint("##course", "e.g. CS 161", &search_text, ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if (submitted || ImGui::Button("Find")) {
                if (const Course* found_course = catalog.find_course(search_text)) set_selected_course(*found_course);
            }
            ImGui::End();

            ImGui::Begin("Course details");
            if (selected_course.course_code.empty()) {
                ImGui::TextDisabled("Select a course node or search by exact course code.");
            } else {
                ImGui::TextColored(ImVec4(0.30f, 0.62f, 1.0f, 1.0f), "%s", selected_course.course_name.c_str());
                ImGui::Text("%s", selected_course.course_code.c_str());
                ImGui::Separator();
                ImGui::TextDisabled("Description"); show_text(selected_course.description);
                ImGui::TextDisabled("Prerequisites"); show_text(selected_course.prerequisites_raw);
                ImGui::TextDisabled("Attributes"); show_text(join(selected_course.attributes));
                ImGui::TextDisabled("Recommended"); show_text(selected_course.recommended);
                ImGui::TextDisabled("Equivalent courses"); show_text(join(selected_course.equivalent));
                if (ImGui::Button("Open prerequisite graph")) open_course_graph();
                const std::string subject = catalog_subject_url(selected_course.subject);
                if (!subject.empty()) {
                    ImGui::SameLine();
                    ImGui::TextLinkOpenURL("Catalog page", ("https://catalog.oregonstate.edu/courses/" + subject).c_str());
                }
            }
            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
            SDL_Delay(1);
        }
    } catch (const std::exception& exception) {
        std::cerr << "Application error: " << exception.what() << '\n';
        result = 1;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
