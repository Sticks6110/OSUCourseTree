#include <iostream>
#include <random>
#include <SDL3/SDL.h>
#include <glad/glad.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <misc/imgui_stdlib.h>

#include "ComputeShader.h"
#include "courses.h"
#include "graph_window.h"
#include "node.h"
#include "Shader.h"

//Some Code Is Borrowed From My Other Project: https://github.com/Sticks6110/SticksEngine/tree/main

int main() {
    std::cout << "Hello, World!" << std::endl;

    //Initialize SDL with Video flag
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL_Init failed!" << std::endl;
        return -1;
    }

    //Set GL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); //RGB8

    //Create a resizable Window
    SDL_Window *window = SDL_CreateWindow("OSU Course Tree", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cout << "SDL_CreateWindow failed!" << std::endl;
        return -1;
    }

    //Create the GL Context and get them ready for rendering
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "assets/layout.ini";

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    //Load Catalog and Generate Graph
    courses* catalog = new courses("assets/osu_courses_2026_2027_processed.json");

    // Create graph window
    graph_window graph_window_display("Graph", catalog->generate_graph(), catalog);

    std::vector<std::unique_ptr<Graph>> graphs;
    std::vector<std::unique_ptr<graph_window>> graph_windows;

    ///
    /// MAIN LOOP
    ///

    //Physics Settings
    bool force_data_dirty = false;
    float repulsion = 30.0f;
    float spring_strength = 0.05f;
    float spring_length = 10.0f;
    float spring_damping = 0.9f;
    float centering_strength = 0.01f;

    // Search Data
    std::string search_string;

    // Course Info View
    std::string info_course_name;
    std::string info_course_code;
    std::string info_course_desc;
    std::string info_course_prereqs;
    std::string info_course_attributes;
    std::string info_course_recommended;
    std::string info_course_equivalent;
    std::string info_course_subject;

    //FPS Tracking
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();
    float time = 0.0;
    bool running = true;
    while (running) {
        ///
        /// FPS LIMITING
        ///
        Uint64 now = SDL_GetPerformanceCounter();
        double delta = (double) (now - last) * 1000.0 / freq;
        float deltaTime = delta / 1000.0;
        time += deltaTime;

        if (delta < 1000.0 / 60.0) {
            SDL_Delay(0);
            continue;
        }

        last = now;

        ///
        /// BASIC DATA
        ///

        // Screen Data
        int w;
        int h;
        SDL_GetWindowSize(window, &w, &h);

        ///
        /// Event Handling
        ///
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            graph_window_display.process_event(e);

            for (auto & graph : graph_windows) {
                graph->process_event(e);
            }

            switch (e.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        float mouse_x = e.button.x;
                        float mouse_y = e.button.y;
                        Course course = graph_window_display.select_node_at_location(mouse_x, mouse_y);

                        if (course.course_code != "") {
                            std::string attributes = std::accumulate(course.attributes.begin(), course.attributes.end(), std::string(""));
                            std::string equivalent = std::accumulate(course.attributes.begin(), course.attributes.end(), std::string(""));

                            info_course_code = course.course_code;
                            info_course_name = course.course_name;
                            info_course_desc = course.description;
                            info_course_prereqs = course.prerequisites_raw;
                            info_course_attributes = attributes;
                            info_course_recommended = course.recommended;
                            info_course_equivalent = equivalent;
                            info_course_subject = course.subject;
                            std::transform(info_course_subject.begin(), info_course_subject.end(), info_course_subject.begin(), [](unsigned char c) {
                                return std::tolower(c);
                            });
                        }
                    }
            }
        }

        // Update physics settings
        if (force_data_dirty) {
            graph_window_display.set_physics_settings(repulsion, spring_strength, spring_length, spring_damping, centering_strength);
            for (auto & graph : graph_windows) {
                graph->set_physics_settings(repulsion, spring_strength, spring_length, spring_damping, centering_strength);
            }
            force_data_dirty = false;
        }

        ///
        /// Rendering
        ///

        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspace_flags);

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::MenuItem("Save Layout")) {
                    ImGui::SaveIniSettingsToDisk("assets/layout.ini");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        graph_window_display.update(deltaTime);
        for (auto & graph : graph_windows) {
            graph->update(deltaTime);
        }

        ImGui::Begin("Physics Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::InputFloat("Repulsion", &repulsion)) force_data_dirty = true;
        if (ImGui::InputFloat("Spring Strength", &spring_strength)) force_data_dirty = true;
        if (ImGui::InputFloat("Spring Length", &spring_length)) force_data_dirty = true;
        if (ImGui::InputFloat("Spring Damping", &spring_damping)) force_data_dirty = true;
        if (ImGui::InputFloat("Centering Strength", &centering_strength)) force_data_dirty = true;
        ImGui::End();

        ImGui::Begin("Search", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputText("##SearchText", &search_string);
        ImGui::Button("Search");
        ImGui::End();

        ImGui::Begin("Information", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(0.19607f, 0.39215f, 1.0f, 1.0f), info_course_name.c_str());
        ImGui::TextWrapped(info_course_code.c_str());
        ImGui::TextWrapped(info_course_desc.c_str());
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.19607f, 0.39215f, 1.0f, 1.0f), "Prerequisites");
        ImGui::TextWrapped(info_course_prereqs.c_str());
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.19607f, 0.39215f, 1.0f, 1.0f), "Attributes");
        ImGui::TextWrapped(info_course_attributes.c_str());
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.19607f, 0.39215f, 1.0f, 1.0f), "Recommended");
        ImGui::TextWrapped(info_course_recommended.c_str());
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.19607f, 0.39215f, 1.0f, 1.0f), "Equivalent");
        ImGui::TextWrapped(info_course_equivalent.c_str());
        ImGui::Separator();

        if (ImGui::Button("Open In Seperate Graph")) {
            graph_windows.emplace_back(std::make_unique<graph_window>(info_course_code, catalog->generate_course_graph(info_course_code), catalog));
        }
        ImGui::TextLinkOpenURL("Open WebPage", ("https://catalog.oregonstate.edu/courses/" + info_course_subject).c_str());

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // cleanup
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}