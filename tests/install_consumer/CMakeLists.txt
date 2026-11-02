# Consumer of an installed hf-at25040-driver: find_package() + the documented target name. Built by
# the hf_at25040_install_consumer test (tests/install_consumer.cmake) against a fresh install.
cmake_minimum_required(VERSION 3.16)
project(hf_at25040_install_consumer LANGUAGES CXX)

find_package(hf_at25040 CONFIG REQUIRED)

add_library(consumer OBJECT main.cpp)
target_link_libraries(consumer PRIVATE hf::at25040)
target_compile_options(consumer PRIVATE -Wall -Wextra -Werror)
