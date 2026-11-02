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
foreach(dir IN LISTS HF_AT25040_PUBLIC_INCLUDE_DIRS)
  target_include_directories(${HF_AT25040_TARGET_NAME} INTERFACE $<BUILD_INTERFACE:${dir}>)
endforeach()
target_include_directories(${HF_AT25040_TARGET_NAME} INTERFACE $<INSTALL_INTERFACE:include>)
target_compile_features(${HF_AT25040_TARGET_NAME} INTERFACE cxx_std_17)

add_library(hf::at25040 ALIAS ${HF_AT25040_TARGET_NAME})
# Installed copies export the same name: hf:: + EXPORT_NAME = hf::at25040.
set_target_properties(${HF_AT25040_TARGET_NAME} PROPERTIES EXPORT_NAME at25040)

# ── Install and export (find_package(hf_at25040) + hf::at25040) ──────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${HF_AT25040_TARGET_NAME} EXPORT hf_at25040Targets)
# Every header the public headers include: at25040.hpp, _registers, _spi_interface and the
# generated at25040_version.h.
install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
install(FILES "${HF_AT25040_VERSION_HEADER}" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT hf_at25040Targets
  FILE hf_at25040Targets.cmake
  NAMESPACE hf::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_at25040
)
configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/hf_at25040Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/hf_at25040Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_at25040
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hf_at25040ConfigVersion.cmake"
  VERSION ${HF_AT25040_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/hf_at25040Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/hf_at25040ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_at25040
)

# ── Host unit tests (mock EEPROM; no hardware) ───────────────────────────────
option(HF_AT25040_BUILD_TESTS "Build the host unit tests in tests/" OFF)
if(HF_AT25040_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
