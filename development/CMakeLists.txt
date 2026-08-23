# =============================================================================
# hf-at25040-driver — Root CMakeLists.txt
# =============================================================================
cmake_minimum_required(VERSION 3.16)

project(hf_at25040
  VERSION 1.0.0
  LANGUAGES CXX
)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_at25040_build_settings.cmake)

add_library(${HF_AT25040_TARGET_NAME} INTERFACE)
target_include_directories(${HF_AT25040_TARGET_NAME} INTERFACE ${HF_AT25040_PUBLIC_INCLUDE_DIRS})
target_compile_features(${HF_AT25040_TARGET_NAME} INTERFACE cxx_std_17)

add_library(hf::at25040 ALIAS ${HF_AT25040_TARGET_NAME})
