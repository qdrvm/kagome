/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace kagome::storage::face {
  template <typename T>
  struct ViewTrait;

  template <typename T>
  using View = typename ViewTrait<T>::type;
}  // namespace kagome::storage::face
