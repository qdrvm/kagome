/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GENERIC_MAPS_HPP
#define KAGOME_GENERIC_MAPS_HPP

#include "storage/face/batch_writeable.hpp"
#include "storage/face/iterable.hpp"
#include "storage/face/readable.hpp"
#include "storage/face/writeable.hpp"

namespace kagome::storage::face {

  /**
   * @brief An abstraction over a readable and iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V, typename KView = K>
  struct ReadOnlyMap
      : public Iterable<K,
                        typename ReadableMap<K, V>::ConstValueView,
                        KView>,
        public ReadableMap<KView, V> {};

  template <typename K, typename V, typename KView = K>
  struct ReadOnlyStorage : public Iterable<K, V, KView>,
                           public ReadableStorage<KView, V> {};

  /**
   * @brief An abstraction over a readable, writeable, iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V, typename KView = K>
  struct GenericMap : public ReadOnlyMap<K, V, KView>,
                      public Writeable<KView, V>,
                      public BatchWriteable<KView, V> {};

  template <typename K, typename V, typename KView = K>
  struct GenericStorage : public ReadOnlyStorage<K, V, KView>,
                          public Writeable<KView, V>,
                          public BatchWriteable<KView, V> {};

}  // namespace kagome::storage::face

#endif  // KAGOME_GENERIC_MAPS_HPP
