#pragma once

#include "app.hpp"

#include <imgui.h>
#include <imgui_memory_editor.h>

#include "decima/archive/archive_manager.hpp"
#include "decima/archive/archive_tree.hpp"
#include "decima/constants.hpp"

class ProjectDS : public App {
public:
    enum class Popup {
        None,
        About,
        AppendExportByName,
        AppendExportByHash,
        Shortcuts
    };

    struct ShortcutInfo {
        std::string_view name;
        std::string_view description;
        std::size_t key;
        ImGuiKeyModFlags mods;
        std::function<void()> callback;
    };

public:
    ProjectDS(const std::pair<uint32_t, uint32_t>& windowSize, const std::string& title,
        bool imgui_multi_viewport = false);

public:
    bool m_multi_viewport;
    Popup current_popup = Popup::None;
    std::vector<ShortcutInfo> shortcuts;

    Decima::ArchiveManager archive_array;
    std::vector<const char*> file_names;
    std::string data_path;
    FileTree root_tree;
    SelectionInfo selection_info;
    ImGuiTextFilter filter;
    MemoryEditor file_viewer;
    bool root_tree_constructing { false };
    bool archive_filter_changed { false };

    void init_user() override;

    void init_imgui();

    void init_filetype_handlers();

    void load_config();
    void save_config();
    void load_data_directory(const std::string& path);

    std::vector<Decima::Archive*> get_selected_archives();

    Decima::OptionalRef<Decima::CoreFile> query_file_from_selected_archives(std::uint64_t hash);

protected:
    void update_user(double ts) override;
    void input_user() override;

    void draw_dockspace();
    void draw_filepreview();
    void draw_tree();
    void draw_export();
    void draw_archive_select();


    void begin_frame_user() override;

    void end_frame_user() override;


};
