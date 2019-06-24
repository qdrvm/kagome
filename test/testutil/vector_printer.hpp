/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_VECTOR_PRINTER_HPP
#define KAGOME_TEST_TESTUTIL_VECTOR_PRINTER_HPP

#include <vector>

namespace boost::detail::variant {
  /**
   * @brief Workaround for gmock unable to print std::vector<unsigned char>
   * @tparam S output stream type
   * @tparam T vector item type
   * @tparam A vector item allocator type
   * @param os output stream reference
   * @param v vector to print
   * @return reference to stream
   */
  template <class S, class T, class A>
  S &operator<<(S &os, const std::vector<T, A> &v) {
    os << "[";
    for (auto &it : v) {
      os << it << ", ";
    }
    os << "] ";
    return os;
  }
}  // namespace boost::detail::variant

#endif  // KAGOME_TEST_TESTUTIL_VECTOR_PRINTER_HPP
