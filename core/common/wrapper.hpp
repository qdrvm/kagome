/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_WRAPPER_HPP
#define KAGOME_CORE_COMMON_WRAPPER_HPP

#include <memory>
#include <type_traits>

#include <boost/operators.hpp>

namespace kagome::common {

  /**
   * @brief Make strongly typed structures from different concepts of the equal
   * types. E.g. block height and round number are both uint64_t, but better to
   * be different types. Or, ID and Signature both vectors.
   * @tparam T wrapped type
   * @tparam Tag unique tag
   */
  template <typename T, typename Tag>
  struct Wrapper {
    explicit Wrapper(T &&t) : data_(std::forward<T>(t)) {}

    T unwrap() {
      return data_;
    }

    const T &unwrap() const {
      return data_;
    }

    T &unwrap_mutable() {
      return data_;
    }

    bool operator==(Wrapper<T, Tag> &other) {
      return data_ == other.data_;
    }

   private:
    T data_;
  };

  template <typename T,
            typename Tag,
            typename = std::enable_if<std::is_arithmetic<T>::value>>
  bool operator<(Wrapper<T, Tag> &a, Wrapper<T, Tag> &b) {
    return a.unwrap() < b.unwrap();
  }

}  // namespace kagome::common

#endif  // KAGOME_CORE_COMMON_WRAPPER_HPP
