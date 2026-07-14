# Copyright © 2026 Lucius Martius
# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 4.3)

project(nmg LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 26)

add_subdirectory(lib)

add_executable(nmg
  main.cpp
)

target_link_libraries(nmg PRIVATE
  lib
)
