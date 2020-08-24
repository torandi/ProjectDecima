//
// Created by i.getsman on 06.08.2020.
//

#ifndef PROJECTDS_PROJECTDS_APP_HPP
#define PROJECTDS_PROJECTDS_APP_HPP

#include "app.hpp"

#include <imgui.h>
#include <imgui_memory_editor.h>

#include "decima/archive/archive_array.h"
#include "decima/archive/archive_tree.h"
#include "decima/constants.hpp"

class ProjectDS : public App {
public:
    enum class Popup {
        None,
        AppendExportByName,
        AppendExportByHash
    };

public:
    ProjectDS(const std::pair<uint32_t, uint32_t>& windowSize, const std::string& title,
        bool imgui_multi_viewport = false);

public:
    bool m_multi_viewport;
    Popup current_popup = Popup::None;

    Decima::ArchiveArray archive_array;
    std::vector<const char*> file_names;
    FileTree root_tree;
    SelectionInfo selection_info;
    std::vector<std::unique_ptr<Decima::CoreEntry>> parsed_files;
    ImGuiTextFilter filter;
    MemoryEditor file_viewer;

    void init_user() override;

    void init_imgui();

    void init_filetype_handlers();

protected:
    void update_user(double ts) override;
    void input_user() override;

    void draw_dockspace();
    void draw_filepreview();
    void draw_tree();
    void draw_export();

    void begin_frame_user() override;

    void end_frame_user() override;

    void parse_core_file();
};

#endif //PROJECTDS_PROJECTDS_APP_HPP
