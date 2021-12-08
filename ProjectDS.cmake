add_executable(ProjectDS
        src/main.cpp
        src/decima/archive/archive.cpp
        src/decima/archive/archive_manager.cpp
        src/decima/archive/archive_tree.cpp
        src/decima/archive/archive_file.cpp
        src/utils.cpp
        src/app.cpp
        src/projectds_app.cpp
        src/projectds_app_draw.cpp
        src/decima/serializable/object/object.cpp
        src/decima/serializable/object/object_draw.cpp
        src/decima/serializable/object/object_dummy.cpp
        src/decima/serializable/object/collection.cpp
        src/decima/serializable/object/collection_draw.cpp
        src/decima/serializable/object/prefetch.cpp
        src/decima/serializable/object/prefetch_draw.cpp
        src/decima/serializable/object/translation.cpp
        src/decima/serializable/object/translation_draw.cpp
        src/decima/serializable/object/texture.cpp
        src/decima/serializable/object/texture_draw.cpp
        src/decima/serializable/object/texture_set.cpp
        src/decima/serializable/object/texture_set_draw.cpp
        src/decima/serializable/reference.cpp
        src/decima/serializable/string.cpp
        src/decima/serializable/stream.cpp
        src/decima/serializable/guid.cpp
        src/decima/serializable/handlers.cpp
        include/decima/archive/archive.hpp
        include/decima/archive/archive_manager.hpp
        include/decima/archive/archive_tree.hpp
        include/decima/archive/archive_file.hpp
        include/utils.hpp
        include/app.hpp
        include/projectds_app.hpp
        include/decima/constants.hpp
		include/decima/shared.hpp
		include/decima/serializable/object/object.hpp
        include/decima/serializable/object/object_dummy.hpp
        include/decima/serializable/object/collection.hpp
        include/decima/serializable/object/prefetch.hpp
        include/decima/serializable/object/translation.hpp
        include/decima/serializable/object/texture.hpp
        include/decima/serializable/object/texture_set.hpp
        include/decima/serializable/reference.hpp
        include/decima/serializable/string.hpp
        include/decima/serializable/stream.hpp
        include/decima/serializable/guid.hpp
        include/decima/serializable/handlers.hpp)

target_link_libraries(ProjectDS PRIVATE oodle hash imgui glfw glad)
target_include_directories(ProjectDS PRIVATE include)

if (MSVC)
    target_compile_definitions(ProjectDS PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(ProjectDS PUBLIC /EHs)
endif ()