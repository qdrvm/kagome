/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_UTIL_HPP
#define KAGOME_APPLICATION_UTIL_HPP

#include <boost/optional.hpp>
#include "application/impl/config_reader/error.hpp"

namespace kagome::application {

  template <typename T>
  outcome::result<std::decay_t<T>> ensure(boost::optional<T> opt_entry) {
    if (not opt_entry) {
      return ConfigReaderError::MISSING_ENTRY;
    }
    return opt_entry.value();
  }

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_UTIL_HPP
