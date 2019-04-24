/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PERSISTED_MAP_HPP
#define KAGOME_PERSISTED_MAP_HPP

#include <boost/filesystem/path.hpp>
#include <outcome/outcome.hpp>
#include "storage/concept/generic_map.hpp"
#include "storage/concept/write_batch.hpp"

namespace kagome::storage::concept {

  template <typename K, typename V>
  struct PersistedMap : public GenericMap<K, V> {
    ~PersistedMap() override = default;

    /**
     * @brief Creates new Write Batch - an object, which can be used to
     * efficiently write bulk data.
     */
    virtual std::unique_ptr<WriteBatch<K, V>> batch() = 0;
  };

}  // namespace kagome::storage::concept

#endif  // KAGOME_PERSISTED_MAP_HPP
