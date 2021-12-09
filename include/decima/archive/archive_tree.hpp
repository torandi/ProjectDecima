#pragma once

#include <unordered_set>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <imgui.h>

#include "archive_manager.hpp"
#include "decima/serializable/object/object_dummy.hpp"

struct SelectionInfo {
    SelectionInfo() = default;
    std::uint64_t preview_file { 0 };
    std::uint64_t preview_file_size { 0 };
    std::uint64_t preview_file_offset { 0 };
    std::uint64_t selected_file { 0 };
    std::unordered_set<std::uint64_t> selected_files;
    std::set<int> selected_archives;
    Decima::CoreFile* file;
};

struct FileInfo {
    uint64_t hash { 0 };
    Decima::CoreHeader header { 0 };
};

template<typename T>
struct FileTreeToggleable {
    FileTreeToggleable(T&& t)
        : tree(std::move(t)) {};
    T tree;
    bool inFilter { true };
    bool inArchives { true };
};

class FileTree {
public:
    std::map<std::string, FileTreeToggleable<std::unique_ptr<FileTree>>> folders;
    std::map<std::string, FileTreeToggleable<FileInfo>> files;

    FileTree* add_folder(const std::string& name);

    void add_file(const std::string& filename, uint64_t hash, Decima::CoreHeader header);

    void update_filter(const ImGuiTextFilter& filter);
    void update_archive_filter(const std::vector<Decima::Archive*>& archives);

    void reset_filter(bool state);
    void reset_archive_filter(bool state);

    template <typename Visitor>
    void visit(const Visitor& visitor, std::size_t depth = 0) const {
        for (const auto& [name, data] : folders) {
            visitor(name, depth);
            data.first->visit(visitor, depth + 1);
        }

        for (const auto& [name, _] : files) {
            visitor(name, depth);
        }
    }

    void draw(SelectionInfo& selection, Decima::ArchiveManager& archive_array, bool draw_header = true);
};
