/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_FACE_OWNED_OR_VIEW_HPP
#define KAGOME_STORAGE_FACE_OWNED_OR_VIEW_HPP

namespace kagome::storage::face {
  template <typename T>
  struct OwnedOrViewTrait;

  template <typename T>
  using OwnedOrView = typename OwnedOrViewTrait<T>::type;
}  // namespace kagome::storage::face

#endif  // KAGOME_STORAGE_FACE_OWNED_OR_VIEW_HPP
