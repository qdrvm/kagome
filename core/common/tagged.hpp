/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/tagged.hpp>

namespace kagome {

  /**
   * The Tagged class wraps an underlying type T with an associated tag.
   * It enables type safety and operator overloading while mimicking the
   * behavior of T.
   *
   * @tparam T The underlying type.
   * @tparam Tag The tag type used for differentiation.
   */
  template <typename T, typename Tag>
  using Tagged = qtils::Tagged<T, Tag>;

}  // namespace kagome
