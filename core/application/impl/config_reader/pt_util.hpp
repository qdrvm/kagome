/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_UTIL_HPP
#define KAGOME_APPLICATION_UTIL_HPP

#include "application/impl/config_reader/error.hpp"
#include "application/impl/kagome_config.hpp"

namespace kagome::application {

  template <typename T>
  outcome::result<std::decay_t<T>> res(boost::optional<T> opt_entry) {
    if (not opt_entry) {
      return ConfigReaderError::MISSING_ENTRY;
    }
    return opt_entry.value();
  }

  outcome::result<std::vector<uint8_t>> unhexWith0x(std::string_view hex);

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_UTIL_HPP
