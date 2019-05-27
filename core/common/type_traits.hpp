/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_TYPE_TRAITS_HPP
#define KAGOME_CORE_COMMON_TYPE_TRAITS_HPP

#include <type_traits>

namespace kagome::common {
  /**
   * @brief removes constness and references from type
   * @tparam T source type
   */
  template <typename T>
  struct remove_cv_ref {
    using type =
        typename std::remove_cv_t<typename std::remove_reference_t<T>>;
  };

  template <typename T>
  using remove_cv_ref_t = typename remove_cv_ref<T>::type;
}  // namespace kagome::common

#endif  // KAGOME_CORE_COMMON_TYPE_TRAITS_HPP
