// Copyright © 2026 Lucius Martius
// SPDX-License-Identifier: GPL-3.0-only

#include "cli.h"

#include <cassert>
#include <print>
#include <string_view>
#include <vector>

int main(int argc, char ** argv) {
    assert(argc >= 1);
    std::vector<std::string_view> args(argv + 1, argv + argc);

    try {
        auto cli = nmg::cli(args);
        cli.run();
    } catch (const std::exception & ex) {
        std::print("Error: {}", ex.what());
    }
}
