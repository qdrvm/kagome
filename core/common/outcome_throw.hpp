/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_OUTCOME_THROW_HPP
#define KAGOME_CORE_COMMON_OUTCOME_THROW_HPP

#include <boost/throw_exception.hpp>
#include <boost/system/system_error.hpp>

namespace kagome::common {
  template <typename T>
  void throwError(const T &t) {
    boost::throw_exception(boost::system::system_error(t));
  }
}  // namespace kagome::common

#endif  // KAGOME_CORE_COMMON_OUTCOME_THROW_HPP
