//
// Created by MED45 on 27.07.2020.
//

#include <filesystem>

#include "decima/archive/archive_tree.hpp"

#include <imgui.h>
#include "utils.hpp"

FileTree* FileTree::add_folder(const std::string& name) {
    if (folders.find(name) == folders.end())
        folders.emplace(name, FileTreeToggleable(std::make_unique<FileTree>()));
    return folders.at(name).tree.get();
}

void FileTree::add_file(const std::string& filename, uint64_t hash, Decima::CoreHeader header) {
    files.emplace(filename, FileTreeToggleable(FileInfo { hash, header }));
}

bool is_filter_matches(FileTree* root, const ImGuiTextFilter& filter) {
    bool result = false;

    for (auto& [name, data] : root->files) {
        if (filter.PassFilter(name.c_str())) {
            data.inFilter = true;
            result = true;
        }
    }

    for (auto& [name, data] : root->folders) {
        if (is_filter_matches(data.tree.get(), filter)) {
            data.inFilter = true;
            result = true;
        }
    }

    return result;
}

bool is_archive_matches(FileTree* root, const std::unordered_map<std::uint64_t, int>& file_filter) {
    bool result = false;

    for (auto& [name, data] : root->files) {
        if (file_filter.count(data.tree.hash) != 0) {
            data.inArchives = true;
            result = true;
        }
    }

    for (auto& [name, data] : root->folders) {
        if (is_archive_matches(data.tree.get(), file_filter)) {
            data.inArchives = true;
            result = true;
        }
    }

    return result;

}

void FileTree::update_filter(const ImGuiTextFilter& filter) {
    if (filter.IsActive()) {
        reset_filter(false);
        is_filter_matches(this, filter);
    } else {
        reset_filter(true);
    }
}

void FileTree::reset_filter(bool state) {
    for (auto& [_, data] : folders) {
        data.tree->reset_filter(state);
        data.inFilter = state;
    }

    for (auto& [_, data] : files) {
        data.inFilter = state;
    }
}

void FileTree::reset_archive_filter(bool state) {
    archive_files.clear();
    for (auto& [_, data] : folders) {
        data.tree->reset_archive_filter(state);
        data.inArchives = state;
    }

    for (auto& [_, data] : files) {
        data.inArchives = state;
    }
}

void FileTree::update_archive_filter(const std::vector<Decima::Archive*>& archives) {
    if (!archives.empty()) {
        reset_archive_filter(false);

        for (const auto* archive: archives) {
            for (const auto& [hash, _] : archive->get_hash_lookup()) {
                archive_files.insert(std::make_pair(hash, archive->index));
            }
        }

        is_archive_matches(this, archive_files);
    } else {
        reset_archive_filter(true);
    }
}


void FileTree::draw(SelectionInfo& selection, Decima::ArchiveManager& archive_array, bool draw_header, FileTree* root) {
    if (draw_header) {
        ImGui::Separator();
        ImGui::Columns(3);
        ImGui::Text("Name");
        ImGui::NextColumn();
        ImGui::Text("Type");
        ImGui::NextColumn();
        ImGui::Text("Size");
        ImGui::NextColumn();
        ImGui::Separator();

        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 200);
        ImGui::SetColumnWidth(1, 100);
        ImGui::SetColumnWidth(2, 100);
    }
    // archive lookup stored in root
    if (root == nullptr)
        root = this;

    for (auto& [name, data] : folders) {
        if (!data.inFilter || !data.inArchives)
            continue;

        const auto tree_name = name + "##" + std::to_string(folders.size());
        const auto show = ImGui::TreeNodeEx(tree_name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick);
        const auto items_count = data.tree->files.size() + data.tree->folders.size();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen() && ImGui::GetIO().KeyCtrl) {
            /*
             * Check whether all files in this folders are selected or not,
             * if selected, then deselect everything, if at least one is not selected,
             * then select everything
             */

            std::unordered_set<std::uint64_t> folder_files;

            for (const auto& file : data.tree->files) {
                folder_files.insert(file.second.tree.hash);
            }

            bool contains_all = true;

            for (const auto& item : folder_files) {
                if (selection.selected_files.find(root->get_selection_info(item)) == selection.selected_files.end()) {
                    contains_all = false;
                    break;
                }
            }

            if (contains_all) {
                for (const auto& item : folder_files)
                    selection.selected_files.erase(root->get_selection_info(item));
            } else {
                for (const auto& item : folder_files)
                    selection.selected_files.insert(root->get_selection_info(item));
            }
        }

        ImGui::NextColumn();
        ImGui::Text("Folder");
        ImGui::NextColumn();
        ImGui::Text("%llu item%c", items_count, items_count == 1 ? ' ' : 's');
        ImGui::NextColumn();

        if (show) {
            if (items_count > 0) {
                data.tree->draw(selection, archive_array, false, root);
            } else {
                ImGui::TextDisabled("Empty");
                ImGui::NextColumn();
                ImGui::NextColumn();
                ImGui::NextColumn();
            }
            ImGui::TreePop();
        }
    }

    for (auto& [name, data] : files) {
        if (!data.inFilter || !data.inArchives)
            continue;

        auto item = root->get_selection_info(data.tree.hash);

        const bool is_selected = selection.selected_files.find(item) != selection.selected_files.end();

        ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_Leaf | static_cast<int>(is_selected));

        if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            selection.selected_file = data.tree.hash;

            if (ImGui::GetIO().KeyCtrl) {
                if (is_selected) {
                    selection.selected_files.erase(item);
                } else {
                    selection.selected_files.insert(item);
                }
            }
        }

        ImGui::TreePop();

        ImGui::NextColumn();
        ImGui::Text("File");
        ImGui::NextColumn();

        const auto file_entry = archive_array.get_file_entry(data.tree.hash);

        if (file_entry.has_value()) {
            ImGui::Text("%s", format_size(file_entry.value().get().size).c_str());
        } else {
            ImGui::Text("Unknown");
        }

        ImGui::NextColumn();
    }

    if (draw_header) {
        ImGui::Columns(1);
    }
}
FileSelectionInfo FileTree::get_selection_info(std::uint64_t hash) {
    auto it = archive_files.find(hash);
    if (it != archive_files.end())
        return FileSelectionInfo(hash, it->second);
    else
        return FileSelectionInfo(hash, -1);
}
