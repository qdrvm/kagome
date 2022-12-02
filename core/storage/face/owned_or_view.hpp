/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_FACE_OWNED_OR_VIEW_HPP
#define KAGOME_STORAGE_FACE_OWNED_OR_VIEW_HPP

namespace kagome::storage::face {
  template <typename T>
  struct OwnedOrView;

  template <typename T>
  using OwnedOrViewOf = typename OwnedOrView<T>::type;
}  // namespace kagome::storage::face

#endif  // KAGOME_STORAGE_FACE_OWNED_OR_VIEW_HPP
