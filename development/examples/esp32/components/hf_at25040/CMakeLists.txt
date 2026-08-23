get_filename_component(HF_AT25040_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HF_AT25040_ROOT}/cmake/hf_at25040_build_settings.cmake")
idf_component_register(INCLUDE_DIRS ${HF_AT25040_PUBLIC_INCLUDE_DIRS})
