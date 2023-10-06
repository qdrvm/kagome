/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace kagome::common {

  struct Uri final {
   public:
    std::string Schema;
    std::string Host;
    std::string Port;
    std::string Path;
    std::string Query;
    std::string Fragment;

    static Uri parse(std::string_view uri);

    std::string to_string() const;

    const std::optional<std::string_view> &error() const {
      return error_;
    }

   private:
    std::optional<std::string_view> error_;
  };

}  // namespace kagome::common
