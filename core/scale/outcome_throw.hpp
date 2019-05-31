/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_OUTCOME_THROW_HPP
#define KAGOME_CORE_COMMON_OUTCOME_THROW_HPP

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include "scale/scale_error.hpp"

namespace kagome::scale::common {

  /**
   * @brief throws outcome::result error as boost exception
   * @tparam T error type
   * @param t error value
   */
  template <typename T>
  void raise(const T &t) {
    boost::throw_exception(std::system_error(make_error_code(t)));
  }
}  // namespace kagome::scale::common

#define OUTCOME_CATCH(expr)                      \
  try {                                          \
    (expr);                                      \
  } catch (boost::system::system_error & errc) { \
    return outcome::failure(errc.code());        \
  }

#endif  // KAGOME_CORE_COMMON_OUTCOME_THROW_HPP
