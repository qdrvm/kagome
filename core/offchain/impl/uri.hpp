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
  public:
   std::string Schema;
   std::string Host;
   std::string Port;
   std::string Path;
   std::string Query;
   std::string Fragment;

   static Uri Parse(std::string_view uri);

   std::string toString() const;

   const std::optional<std::string_view>& error() const {
     return error_;
   }

  private:
   std::optional<std::string_view> error_;
 };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_URI
