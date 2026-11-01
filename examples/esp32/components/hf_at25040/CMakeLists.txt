# hf_at25040 — ESP-IDF component wrapper (header-only driver)
set(HF_AT25040_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." CACHE PATH
    "Path to hf-at25040-driver root")

get_filename_component(HF_AT25040_ROOT "${HF_AT25040_ROOT}" ABSOLUTE)
if(NOT EXISTS "${HF_AT25040_ROOT}/cmake/hf_at25040_build_settings.cmake")
    message(FATAL_ERROR
        "Cannot find AT25040 driver root at ${HF_AT25040_ROOT}")
endif()

include("${HF_AT25040_ROOT}/cmake/hf_at25040_build_settings.cmake")

idf_component_register(
    INCLUDE_DIRS ${HF_AT25040_PUBLIC_INCLUDE_DIRS}
    REQUIRES ${HF_AT25040_IDF_REQUIRES}
)

target_compile_features(${COMPONENT_LIB} INTERFACE cxx_std_17)
