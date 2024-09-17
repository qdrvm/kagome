/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include "outcome/outcome.hpp"

namespace kagome::common {
  /**
   * @brief throws outcome::result error as boost exception
   * @tparam T enum error type, only outcome::result enums are allowed
   * @param t error value
   */
  template <typename T>
    requires std::is_enum_v<T>
  void raise(T t) {
    std::error_code ec = make_error_code(t);
    boost::throw_exception(std::system_error(ec));
  }

  /**
   * @brief throws outcome::result error made of error as boost exception
   * @tparam T outcome error type
   * @param t outcome error value
   */
  template <typename T>
    requires(not std::is_enum_v<T>)
  void raise(const T &t) {
    boost::throw_exception(std::system_error(t.value(), t.category()));
  }

  template <typename T>
  void raise_on_err(const outcome::result<T> &res) {
    if (res.has_error()) {
      raise(res.error());
    }
  }
}  // namespace kagome::common
