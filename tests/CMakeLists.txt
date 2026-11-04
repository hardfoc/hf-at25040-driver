# =============================================================================
# hf-at25040-driver — host unit tests
# =============================================================================
# Builds the driver header against a model of the AT25040B and runs the tests
# with CTest. No hardware and no ESP-IDF needed:
#
#   cmake -S . -B build-host -DHF_AT25040_BUILD_TESTS=ON
#   cmake --build build-host
#   ctest --test-dir build-host --output-on-failure
# =============================================================================

add_executable(hf_at25040_host_tests
  test_device.cpp
)

target_link_libraries(hf_at25040_host_tests PRIVATE ${HF_AT25040_TARGET_NAME})
target_include_directories(hf_at25040_host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(hf_at25040_host_tests PRIVATE cxx_std_17)
target_compile_options(hf_at25040_host_tests PRIVATE -Wall -Wextra)

add_test(NAME hf_at25040_host_tests COMMAND hf_at25040_host_tests)

# Bus errors are reported as Status::BusError, distinct from Timeout and Absent.
add_executable(hf_at25040_bus_error_tests test_bus_errors.cpp)
target_link_libraries(hf_at25040_bus_error_tests PRIVATE ${HF_AT25040_TARGET_NAME})
target_include_directories(hf_at25040_bus_error_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(hf_at25040_bus_error_tests PRIVATE cxx_std_17)
target_compile_options(hf_at25040_bus_error_tests PRIVATE -Wall -Wextra)
add_test(NAME hf_at25040_bus_error_tests COMMAND hf_at25040_bus_error_tests)

# The code shown on the documentation site (docs/ includes these files with pymdownx.snippets), built so a
# page cannot show code that does not compile against the current headers. Compiled, never linked or run.
add_library(hf_at25040_doc_snippets OBJECT
  docs/quickstart.cpp
  docs/api_tasks.cpp
  docs/ports.cpp
)
target_link_libraries(hf_at25040_doc_snippets PRIVATE ${HF_AT25040_TARGET_NAME})
target_include_directories(hf_at25040_doc_snippets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/docs)
target_compile_features(hf_at25040_doc_snippets PRIVATE cxx_std_17)
target_compile_options(hf_at25040_doc_snippets PRIVATE -Wall -Wextra)

# `cmake --install` must ship every header the public headers include (the generated
# at25040_version.h too) and export the documented target hf::at25040. Installs into the build tree
# and builds a consumer with find_package().
add_test(NAME hf_at25040_install_consumer
  COMMAND ${CMAKE_COMMAND} -DBIN=${PROJECT_BINARY_DIR} -DSRC=${PROJECT_SOURCE_DIR}
          -DCXX=${CMAKE_CXX_COMPILER} -P ${CMAKE_CURRENT_SOURCE_DIR}/install_consumer.cmake)
