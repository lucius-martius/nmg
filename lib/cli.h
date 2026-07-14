// Copyright © 2026 Lucius Martius
// SPDX-License-Identifier: GPL-3.0-only

#include <functional>
#include <string_view>
#include <vector>

namespace nmg {

class cli {
  public:
    cli(const std::vector<std::string_view> & args);

    int run() { return command(); }

  private:
    std::move_only_function<int()> command;
};

} // namespace nmg
