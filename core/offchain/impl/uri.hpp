/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_URI
#define KAGOME_OFFCHAIN_URI

#include <optional>
#include <string>
#include <string_view>

namespace kagome::offchain {
  struct Uri final {
   private:
    std::string uri_;
    std::optional<std::string> error_ = "Is not initialized";

   public:
    std::string_view Schema;
    std::string_view Host;
    std::string_view Port;
    std::string_view Path;
    std::string_view Query;
    std::string_view Fragment;

    Uri() = default;
    Uri(const Uri &other);
    Uri(Uri &&other) noexcept;

    Uri &operator=(const Uri &other);
    Uri &operator=(Uri &&other) noexcept;

    const std::string &toString() const {
      return uri_;
    }

    const std::optional<std::string> error() const {
      return error_;
    }

    static Uri Parse(std::string_view uri);
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_URI
