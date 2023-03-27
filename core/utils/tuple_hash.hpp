/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tuple>
#include <functional>
#include <boost/container_hash/hash.hpp>

template <class... Args>
struct std::hash<std::tuple<Args...>> {
 private:
  template <size_t I>
  void hash_element_of_tuple(size_t &result,
                             const std::tuple<Args...> &v) const {
    auto item = std::get<I>(v);
    boost::hash_combine(result, std::hash<decltype(item)>()(item));
    if constexpr (sizeof...(Args) > I + 1) {
      hash_element_of_tuple<I + 1>(result, v);
    }
  }

 public:
  size_t operator()(const std::tuple<Args...> &value) const {
    size_t result = 0;
    if constexpr (sizeof...(Args) > 0) {
      hash_element_of_tuple<0>(result, value);
    }
    return result;
  }
};
