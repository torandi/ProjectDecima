#pragma once

#include "decima/file_types/core/entry.h"
#include "decima/file_types/pod/reference.hpp"

namespace Decima {
    class Collection : public CoreEntry {
    public:
        void parse(ArchiveArray& archives, Source& stream, CoreFile* core_file) override;
        void draw() override;

        std::vector<Reference> refs;
    };
}