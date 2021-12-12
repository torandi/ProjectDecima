//
// Created by i.getsman on 06.08.2020.
//

#include "projectds_app.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <fstream>
#include <filesystem>
#include <GLFW/glfw3.h>


ProjectDS* ProjectDS::s_app = nullptr;

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();

    const char* key_name = glfwGetKeyName(key, scancode);

    if (key_name)
    {
        key = (int)key_name[0];

        // Convert to upper-case
        if (key >= 97 && key <= 122)
            key -= 32;  
    }

    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
#ifdef _WIN32
    io.KeySuper = false;
#else
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
#endif

}

ProjectDS::ProjectDS(const std::pair<uint32_t, uint32_t>& windowSize, const std::string& title,
                     bool imgui_multi_viewport) : App(windowSize,
                                                      title) {
    m_multi_viewport = imgui_multi_viewport;
    s_app = this;
}

void ProjectDS::begin_frame_user() {
    App::begin_frame_user();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

}

void ProjectDS::end_frame_user() {
    App::end_frame_user();
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void ProjectDS::load_config() {
	std::ifstream file("projectds.cfg");
    if (file.is_open()) {
		std::string line;
        while (std::getline(file, line)) {
			int split = line.find_first_of('=');
			std::string key = line.substr(0, split);
			std::string value = line.substr(split + 1);
			if (key == "path") {
				data_path = value;
			}
        }
        file.close();
    }

    if (!data_path.empty())
        load_data_directory(data_path);
}

void ProjectDS::save_config() {
    std::ofstream file;
    file.open("projectds.cfg");
    file << "path=" << data_path << std::endl;
    file.close();
}

void ProjectDS::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    if (m_multi_viewport) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);

    // Override keycallback, to map to real layout
    glfwSetKeyCallback(m_window, KeyCallback);

    ImGui_ImplOpenGL3_Init("#version 150");
    ImGui_ImplOpenGL3_NewFrame();
}

std::vector<Decima::Archive*> ProjectDS::get_selected_archives() {
    std::vector<Decima::Archive*> archives;
    archives.reserve(selection_info.selected_archives.size());
    for (auto it = selection_info.selected_archives.rbegin(); it != selection_info.selected_archives.rend(); ++it) {
        archives.push_back(&archive_array.manager[*it]);
    }
    return archives;
}

Decima::OptionalRef<Decima::CoreFile> ProjectDS::query_file_from_selected_archives(std::uint64_t hash) {
    std::vector<Decima::Archive*> archives = get_selected_archives();
    if (archives.empty())
        return archive_array.query_file(hash);

    for (auto* archive : archives) {
        auto file = archive->query_file(hash);
        if (file.has_value()) {
            return file;
        }
    }
    return {};
}






