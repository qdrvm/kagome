/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/pt_util.hpp"

#include "common/hexutil.hpp"

namespace kagome::application {

  outcome::result<std::vector<uint8_t>> unhexWith0x(
      std::string_view hex_with_prefix) {
    const static std::string leading_chrs = "0x";

    auto without_prefix = hex_with_prefix.substr(
        hex_with_prefix.find(leading_chrs) + leading_chrs.length(),
        hex_with_prefix.length() - 1);
    return common::unhex(without_prefix);
  }

}  // namespace kagome::application
