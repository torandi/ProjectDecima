//
// Created by i.getsman on 06.08.2020.
//

#include "projectds_app.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

ProjectDS::ProjectDS(const std::pair<uint32_t, uint32_t>& windowSize, const std::string& title,
                     bool imgui_multi_viewport) : App(windowSize,
                                                      title) {
    m_multi_viewport = imgui_multi_viewport;
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

void ProjectDS::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    if (m_multi_viewport) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
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






