//
// Created by MED45 on 06.08.2020.
//

#include "decima/file_types/dummy.h"

void Decima::Dummy::parse(Source& stream) {
    CoreFile::parse(stream);
    stream.seek(ash::seek_dir::cur, header.file_size - sizeof(Decima::GUID));
}
