//
// Created by i.getsman on 06.08.2020.
//

#include <filesystem>
#include <fstream>
#include <set>

#include "projectds_app.hpp"

#include "utils.hpp"
#include "decima/constants.hpp"
#include "util/pfd.h"

static void show_data_selection_dialog(ProjectDS& self) {
    auto folder = pfd::select_folder("Select Horizon Zero Dawn Packed_DX12 folder").result();

    if (!folder.empty()) {

        // Sort bin files alphabeticaly
        std::set<std::filesystem::path> sortedArchiveNames;
        for (auto file : std::filesystem::directory_iterator(folder)) {
            // ignore Aloy's adjustments, it's causing issues with the prefetch
            if(file.path().stem() != "Patch_AloysAdjustments")
                sortedArchiveNames.insert(file.path());
        }

        // Iterate in reverse alphabetical order
        for (auto it = sortedArchiveNames.rbegin(); it != sortedArchiveNames.rend(); ++it) {
            LOG("Loading archive ", it->stem());
            int index = self.archive_array.load_archive(it->string());
        }

        self.archive_array.load_prefetch();

        self.file_names.clear();
        self.file_names.reserve(self.archive_array.hash_to_name.size());

        std::thread([](ProjectDS& self) {
            self.root_tree_constructing = true;

            for (const auto& [hash, path] : self.archive_array.hash_to_name) {
                self.file_names.push_back(path.c_str());

                std::vector<std::string> split_path;
                split(path, split_path, '/');

                auto* current_root = &self.root_tree;

                for (auto it = split_path.begin(); it != split_path.end() - 1; it++)
                    current_root = current_root->add_folder(*it);

                if (self.archive_array.hash_to_archive_index.find(hash) != self.archive_array.hash_to_archive_index.end())
                    current_root->add_file(split_path.back(), hash, { 0 });
            }

            self.root_tree_constructing = false;
        },
            std::ref(self))
            .detach();
    }
}

static void show_export_selection_dialog(ProjectDS& self) {
    if (self.selection_info.selected_files.empty())
        return;

    const auto base_folder = pfd::select_folder("Choose destination folder").result();

    if (!base_folder.empty()) {
        for (const auto selected_file : self.selection_info.selected_files) {
            const auto filename = sanitize_name(self.archive_array.hash_to_name.at(selected_file.file_hash));

            std::filesystem::path full_path = std::filesystem::path(base_folder) / filename;
            std::filesystem::create_directories(full_path.parent_path());

            Decima::OptionalRef<Decima::CoreFile> file = {};
            if (selected_file.archive_index != -1) {
                std::uint64_t file_hash = hash_string(sanitize_name(filename), Decima::seed);
                file = self.archive_array.manager[selected_file.archive_index].query_file(file_hash);
            } else {
                file = self.archive_array.query_file(filename);
            }

            if (file.has_value()) {
                std::ofstream output_file { full_path, std::ios::binary };
                output_file.write(reinterpret_cast<const char*>(file.value().get().contents.data()), file.value().get().contents.size());

                std::cout << "File was exported to: " << full_path << " from archive " << file.value().get().get_archive().path << "\n";
            } else {
                std::cout << "Could not export " << filename << ", not found in archive.";
            }
        }
    }
}

static void add_selected_to_export (ProjectDS& self) {
    if (self.selection_info.file == nullptr)
        return;

	std::string filename = self.archive_array.get_file_name(self.selection_info.selected_file);

    if (filename.empty())
		return;

	uint64_t file_hash = hash_string(filename, Decima::seed);
	if (self.archive_array.get_file_entry(file_hash).has_value()) {
		self.archive_array.hash_to_name[file_hash] = filename;
		self.selection_info.selected_files.insert(FileSelectionInfo(file_hash, -1));
	}
}

void ProjectDS::init_user() {
    App::init_user();
    init_imgui();
    init_filetype_handlers();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    file_viewer.WriteFn = [](auto, auto, auto) {
        /* Dummy write function because
         * ReadOnly forbids any selection. */
    };

    shortcuts.push_back(ShortcutInfo {
        "Escape",
        "Reset highlighted selection in raw view",
        GLFW_KEY_ESCAPE,
        ImGuiKeyModFlags_None,
        [&] {
            if (selection_info.preview_file != 0) {
                selection_info.preview_file_size = selection_info.file->contents.size();
                selection_info.preview_file_offset = 0;
            }
        },
    });

    shortcuts.push_back(ShortcutInfo {
        "Ctrl+O",
        "Open file dialog to select game folder",
        GLFW_KEY_O,
        ImGuiKeyModFlags_Ctrl,
        [&] { show_data_selection_dialog(*this); },
    });

    shortcuts.push_back(ShortcutInfo {
        "Ctrl+E",
        "Export currently selected files",
        GLFW_KEY_E,
        ImGuiKeyModFlags_Ctrl,
        [&] { show_export_selection_dialog(*this); },
    });

    shortcuts.push_back(ShortcutInfo {
        "Ctrl+P",
        "Expand all",
        GLFW_KEY_P,
        ImGuiKeyModFlags_Ctrl,
        [&] { root_tree.expand_all(); },
    });

    shortcuts.push_back(ShortcutInfo {
        "Ctrl+Shift+P",
        "Collapse all",
        GLFW_KEY_P,
        ImGuiKeyModFlags_Ctrl | ImGuiKeyModFlags_Shift,
        [&] { root_tree.collapse_all(); },
    });

    shortcuts.push_back(ShortcutInfo {
        "Ctrl+A",
        "Add selected file to selection.",
        GLFW_KEY_A,
        ImGuiKeyModFlags_Ctrl,
        [&] { add_selected_to_export(*this); },
    });

}

void ProjectDS::input_user() {
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureKeyboard)
        return;

    for (const auto& shortcut : shortcuts) {
        if ((io.KeyMods & shortcut.mods) == shortcut.mods && io.KeysDown[shortcut.key])
            shortcut.callback();
    }
}

void ProjectDS::update_user(double ts) {
    App::update_user(ts);
    draw_dockspace();
    draw_filepreview();
    draw_export();
    draw_tree();
    draw_archive_select();
}

void ProjectDS::init_filetype_handlers() {
}

void ProjectDS::draw_dockspace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
    const auto dock_flags = ImGuiWindowFlags_MenuBar
        | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
        | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("DockSpace", nullptr, dock_flags);
    {
        ImGui::PopStyleVar(3);
        ImGui::DockSpace(ImGui::GetID("Dock"));

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open archive", "Ctrl+O")) {
                    show_data_selection_dialog(*this);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About"))
                    current_popup = Popup::About;

                if (ImGui::MenuItem("Shortcuts"))
                    current_popup = Popup::Shortcuts;

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }
    ImGui::End();

    if (current_popup == Popup::Shortcuts) {
        ImGui::OpenPopup("Shortcuts");
        current_popup = Popup::None;
    }

    if (current_popup == Popup::About) {
        ImGui::OpenPopup("About");
        current_popup = Popup::None;
    }

    ImGui::SetNextWindowSize({ 500, 0 }, ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Shortcuts", nullptr, ImGuiWindowFlags_NoMove)) {
        ImGui::Columns(2);
        {
            ImGui::Text("Name");
            ImGui::SetColumnWidth(-1, 150);
            ImGui::NextColumn();

            ImGui::Text("Description");
            ImGui::NextColumn();

            ImGui::Separator();

            for (const auto& shortcut : shortcuts) {
                ImGui::TextUnformatted(shortcut.name.data());
                ImGui::NextColumn();

                ImGui::TextWrapped("%s", shortcut.description.data());
                ImGui::NextColumn();

                ImGui::Separator();
            }
        }
        ImGui::Columns(1);

        if (ImGui::Button("Got it", { -1, 0 }))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::SetNextWindowSize({ 600, 0 }, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, { 0.5, 0.5 });

    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_NoMove)) {
        ImGui::Text("ProjectDecima " DECIMA_VERSION " (" __DATE__ ")");
        ImGui::Separator();

        ImGui::TextWrapped("This project is aimed to provide GUI for export/preview files inside Decima engine manager.");

        if (ImGui::TreeNode("Show license")) {
            ImGui::Text(R"(MIT License

Copyright (c) 2020 REDxEYE & ShadelessFox

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)");
            ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::Button("Source code", { -1, 0 }))
            std::system("start https://github.com/REDxEYE/ProjectDecima");

        if (ImGui::Button("Report problem", { -1, 0 }))
            std::system("start https://github.com/REDxEYE/ProjectDecima/issues");

        ImGui::Separator();

        if (ImGui::Button("Close", { -1, 0 }))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void ProjectDS::draw_filepreview() {
    ImGui::Begin("File preview");
    {
        if (selection_info.selected_file > 0) {
            const auto file_entry_opt = archive_array.get_file_entry(selection_info.selected_file);

            if (file_entry_opt.has_value()) {
                const auto& file_entry = file_entry_opt.value().get();

                std::string filename;
                if (archive_array.hash_to_name.find(selection_info.selected_file) != archive_array.hash_to_name.end()) {
                    filename = sanitize_name(archive_array.hash_to_name.at(selection_info.selected_file));
                } else {
                    filename = uint64_to_hex(selection_info.selected_file);
                }
                ImGui::TextWrapped("%s", filename.c_str());

                if (ImGui::BeginPopupContextItem("File preview name")) {
                    if (ImGui::Selectable("Copy path"))
                        ImGui::SetClipboardText(filename.c_str());
                    ImGui::EndPopup();
                }

                const bool selected_file_changed = selection_info.preview_file != selection_info.selected_file;

                if (selected_file_changed) {
                    selection_info.file = &query_file_from_selected_archives(selection_info.selected_file).value().get();
                    if(!ends_with(archive_array.get_file_name(selection_info.selected_file), "stream")) // don't parse stream files
                        selection_info.file->parse(archive_array);

                    selection_info.preview_file = selection_info.selected_file;
                    selection_info.preview_file_size = selection_info.file->contents.size();
                    selection_info.preview_file_offset = 0;
                }

                ImGui::Separator();
                ImGui::Columns(2);
                {
                    ImGui::Text("Archive ID");
                    ImGui::NextColumn();

                    ImGui::Text("%s", selection_info.file->get_archive().path.c_str());
                    ImGui::NextColumn();

                    ImGui::Separator();

                    ImGui::Text("Size");
                    ImGui::NextColumn();

                    ImGui::Text("%u bytes", file_entry.size);
                    ImGui::NextColumn();

                    ImGui::Separator();

                    ImGui::Text("Hash");
                    ImGui::NextColumn();

                    ImGui::Text("%llX", file_entry.hash);
                    ImGui::NextColumn();

                    ImGui::Separator();

                    ImGui::Text("Entry ID");
                    ImGui::NextColumn();

                    ImGui::Text("%u", file_entry.entry_num);
                    ImGui::NextColumn();

                    ImGui::Separator();

                    ImGui::Text("Offset");
                    ImGui::NextColumn();

                    ImGui::Text("%llu", file_entry.offset);
                    ImGui::Separator();
                }
                ImGui::Columns(1);

            } else {
                ImGui::Text("Error getting file info!");
            }
        } else {
            ImGui::Text("No file selected");
        }

        ImGui::DockSpace(ImGui::GetID("Dock2"));
    }
    ImGui::End();

    ImGui::Begin("Normal View", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (selection_info.selected_file > 0) {
            for (const auto& file : selection_info.file->objects) {
                std::stringstream buffer;
                buffer << '[' << Decima::to_string(file->guid) << "] " << Decima::get_type_name(file->header.file_type);

                const bool opened = ImGui::TreeNode(buffer.str().c_str());

                if (ImGui::BeginPopupContextItem(buffer.str().c_str())) {
                    if (ImGui::Selectable("Highlight")) {
                        selection_info.preview_file_offset = file->offset;
                        selection_info.preview_file_size = file->header.file_size + sizeof(Decima::CoreHeader);
                    }

                    ImGui::EndPopup();
                }

                if (opened) {
                    file->draw();
                    ImGui::TreePop();
                }

                ImGui::Separator();
            }
        }
    }
    ImGui::End();

    ImGui::Begin("Raw View", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (selection_info.selected_file > 0) {
            file_viewer.DrawContents(
                selection_info.file->contents.data() + selection_info.preview_file_offset,
                selection_info.preview_file_size,
                0);
        }
    }
    ImGui::End();
}

void ProjectDS::draw_tree() {
    ImGui::Begin("Trees");
    {
        if (archive_filter_changed) {
            archive_filter_changed = false;
            root_tree.update_archive_filter(get_selected_archives());
        }

        if (filter.Draw()) {
            root_tree.update_filter(filter);

            file_names.clear();

            for (auto& [_, path] : archive_array.hash_to_name) {
                if (filter.PassFilter(path.c_str())) {
                    file_names.push_back(path.c_str());
                }
            }
        }

        if (ImGui::SmallButton("Expand all")) {
            root_tree.expand_all();
        }

        ImGui::SameLine();

        if (ImGui::SmallButton("Collapse all")) {
            root_tree.collapse_all();
        }

        if (root_tree_constructing) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff99ffff);
            ImGui::TextWrapped("File tree is still constructing, some files may be missing");
            ImGui::PopStyleColor();
        }

        ImGui::BeginChild("FileTree");
        root_tree.draw(selection_info, archive_array);
        ImGui::EndChild();
    }
    ImGui::End();
}

void ProjectDS::draw_archive_select() {
    static int last_select = -1;
    ImGui::Begin("Archive Browser");
    {
        if (ImGui::Button("Clear filter", { -1, 0 })) {
            selection_info.selected_archives.clear();
            last_select = -1;
            archive_filter_changed = true;
        }

        if (ImGui::PushItemWidth(-1), ImGui::ListBoxHeader("##", { 0, -1 })) {

            ImGuiIO& io = ImGui::GetIO();

            for (int i=archive_array.manager.size() - 1; i >= 0; --i) {
                const auto& archive = archive_array.manager[i];
                std::filesystem::path path(archive.path);
                const bool is_selected = selection_info.selected_archives.count(i) != 0;
                if(ImGui::Selectable(path.stem().string().c_str(), is_selected))
				{
                    if (io.KeyMods & ImGuiKeyModFlags_Shift) {
                        if (last_select != -1) {
                            int minVal = min(last_select, i);
                            int maxVal = max(last_select, i);
                            for (int j = minVal; j < maxVal + 1; ++j) {
                                selection_info.selected_archives.insert(j);
                            }
                        } else {
                            selection_info.selected_archives.insert(i);
                        }
                    } else if (io.KeyMods & ImGuiKeyModFlags_Ctrl) {
                        auto result = selection_info.selected_archives.insert(i);
                        if (!result.second)
                            selection_info.selected_archives.erase(result.first); // erase if already in selection
                    } else {
						selection_info.selected_archives.clear();
                        selection_info.selected_archives.insert(i);
                    }

                    last_select = i;
                    archive_filter_changed = true;
				}
            }

            ImGui::ListBoxFooter();
        }
    }
    ImGui::End();
}

void ProjectDS::draw_export() {
    ImGui::Begin("Export");
    {
        if (ImGui::SmallButton("Add by name"))
            current_popup = Popup::AppendExportByName;

        ImGui::SameLine();

        if (ImGui::SmallButton("Add by hash"))
            current_popup = Popup::AppendExportByHash;

        ImGui::SameLine();

        if (ImGui::SmallButton("Clear"))
            selection_info.selected_files.clear();

        if (ImGui::Button("Export selected items", { -1, 0 }))
            show_export_selection_dialog(*this);

        if (ImGui::PushItemWidth(-1), ImGui::ListBoxHeader("##", { 0, -1 })) {
            for (const auto& selected_file : selection_info.selected_files) {
                std::string archive_name = "";
                
                if (selected_file.archive_index != -1) {
                    std::filesystem::path archive_path(archive_array.manager[selected_file.archive_index].path);
                    archive_name = "[" + archive_path.stem().string() + "] ";
                }
                if (archive_array.hash_to_name.find(selected_file.file_hash) != archive_array.hash_to_name.end()) {
                    if (ImGui::Selectable((archive_name + archive_array.hash_to_name[selected_file.file_hash]).c_str()))
                        selection_info.selected_file = selected_file.file_hash;
                } else {
                    std::string new_name = archive_name + "Hash: " + uint64_to_hex(selected_file.file_hash);
                    if (ImGui::Selectable(new_name.c_str()))
                        selection_info.selected_file = selected_file.file_hash;
                }
            }

            ImGui::ListBoxFooter();
        }

        if (current_popup == Popup::AppendExportByName) {
            ImGui::OpenPopup("AppendExportByName");
            current_popup = Popup::None;
        }

        if (current_popup == Popup::AppendExportByName) {
            ImGui::OpenPopup("AppendExportByName");
            current_popup = Popup::None;
        }

        if (ImGui::BeginPopup("AppendExportByName")) {
            static char path[512];

            ImGui::InputText("File name", path, IM_ARRAYSIZE(path) - 1);

            if (ImGui::Button("Add to selection!")) {
                std::string str_path(path);
                uint64_t file_hash = hash_string(sanitize_name(str_path), Decima::seed);
                if (archive_array.get_file_entry(file_hash).has_value()) {
                    archive_array.hash_to_name[file_hash] = str_path;
                    selection_info.selected_files.insert(FileSelectionInfo(file_hash, -1));
                }
            }

            ImGui::EndPopup();
        }

        if (current_popup == Popup::AppendExportByHash) {
            ImGui::OpenPopup("AppendExportByHash");
            current_popup = Popup::None;
        }

        if (ImGui::BeginPopup("AppendExportByHash")) {
            static std::uint64_t file_hash = 0;

            ImGui::InputScalar("File hash", ImGuiDataType_U64, &file_hash);

            if (ImGui::Button("Add to selection!")) {
                if (archive_array.get_file_entry(file_hash).has_value()) {
                    archive_array.hash_to_name[file_hash] = "HASH: " + uint64_to_hex(file_hash);
                    selection_info.selected_files.insert(FileSelectionInfo(file_hash, -1));
                }
            }

            ImGui::EndPopup();
        }
    }
    ImGui::End();
}
