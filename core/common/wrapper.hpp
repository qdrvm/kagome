/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

    bool operator==(const Wrapper<T, Tag> &other) const {
      return data_ == other.data_;
    }

   private:
    T data_;
  };

  template <typename T,
            typename Tag,
            typename = std::enable_if<std::is_arithmetic<T>::value>>
  bool operator<(const Wrapper<T, Tag> &a, const Wrapper<T, Tag> &b) {
    return a.unwrap() < b.unwrap();
  }

}  // namespace kagome::common

template <typename T, typename Tag>
struct std::hash<kagome::common::Wrapper<T, Tag>> {
  std::size_t operator()(const kagome::common::Wrapper<T, Tag> &w) {
    return std::hash<T>()(w.unwrap());
  }
};

#endif  // KAGOME_CORE_COMMON_WRAPPER_HPP
