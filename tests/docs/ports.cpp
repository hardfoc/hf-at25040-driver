/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented port, so every member compiles.
 */
#include "at25040.hpp"
#include "my_spi.hpp"

template class at25040::Device<MySpi>;
