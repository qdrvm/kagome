/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_URI
#define KAGOME_OFFCHAIN_URI

#include <string>
#include <string_view>

namespace kagome::offchain {
  struct Uri final {
   public:
    std::string_view Schema;
    std::string_view Host;
    std::string_view Port;
    std::string_view Path;
    std::string_view Query;
    std::string_view Fragment;

    const std::string& toString() const {
      return uri_;
    }

    static Uri Parse(std::string_view uri);

   private:
    std::string uri_;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_URI
