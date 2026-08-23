#===============================================================================
# AT25040 Driver - Build Settings
# Header-only SPI EEPROM. Single source of truth for the driver version.
# Copy this file's pattern for future AFE parts (PGA, DAC).
#===============================================================================

include_guard(GLOBAL)

set(HF_AT25040_TARGET_NAME "hf_at25040")

set(HF_AT25040_VERSION_MAJOR 1)
set(HF_AT25040_VERSION_MINOR 0)
set(HF_AT25040_VERSION_PATCH 0)
set(HF_AT25040_VERSION "${HF_AT25040_VERSION_MAJOR}.${HF_AT25040_VERSION_MINOR}.${HF_AT25040_VERSION_PATCH}")
set(HF_AT25040_VERSION_STRING "${HF_AT25040_VERSION}")

set(HF_AT25040_VERSION_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/../inc/at25040_version.h.in")
set(HF_AT25040_VERSION_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/hf_at25040_generated")
set(HF_AT25040_VERSION_HEADER     "${HF_AT25040_VERSION_HEADER_DIR}/at25040_version.h")

file(MAKE_DIRECTORY "${HF_AT25040_VERSION_HEADER_DIR}")

if(EXISTS "${HF_AT25040_VERSION_TEMPLATE}")
    configure_file(
        "${HF_AT25040_VERSION_TEMPLATE}"
        "${HF_AT25040_VERSION_HEADER}"
        @ONLY
    )
    message(STATUS "AT25040 driver v${HF_AT25040_VERSION} — generated at25040_version.h")
else()
    message(WARNING "at25040_version.h.in not found at ${HF_AT25040_VERSION_TEMPLATE}")
endif()

set(HF_AT25040_PUBLIC_INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../inc"
    "${HF_AT25040_VERSION_HEADER_DIR}"
)

set(HF_AT25040_SOURCE_FILES "")

set(HF_AT25040_IDF_REQUIRES driver)
