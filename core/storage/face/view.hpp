/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_FACE_VIEW_HPP
#define KAGOME_STORAGE_FACE_VIEW_HPP

namespace kagome::storage::face {
  template <typename T>
  struct ViewTrait;

  template <typename T>
  using View = typename ViewTrait<T>::type;
}  // namespace kagome::storage::face

#endif  // KAGOME_STORAGE_FACE_VIEW_HPP
