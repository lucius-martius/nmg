# Copyright © 2026 Lucius Martius
# SPDX-License-Identifier: GPL-3.0-only

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(lib OBJECT
  cli.cpp
  intensitymap.cpp
  normalmapgenerator.cpp
  specularmapgenerator.cpp
  gaussianblur.cpp
  boxblur.cpp
  ssaogenerator.cpp
)

target_link_libraries(lib PUBLIC
  Qt6::Core
  Qt6::Gui
)

target_include_directories(lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
