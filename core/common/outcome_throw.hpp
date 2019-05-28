/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_OUTCOME_THROW_HPP
#define KAGOME_CORE_COMMON_OUTCOME_THROW_HPP

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

namespace kagome::common {

  /**
   * @brief throws outcome::result error as boost exception
   * @tparam T error type
   * @param t error value
   */
  template <typename T>
  void raise(const T &t) {
    boost::throw_exception(std::system_error(make_error_code(t)));
  }
}  // namespace kagome::common::exception

#endif  // KAGOME_CORE_COMMON_OUTCOME_THROW_HPP
