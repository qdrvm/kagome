/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "outcome/outcome.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/spaces.hpp"

namespace kagome::storage {

  /// Spaced storages base interface
  class SpacedStorage {
   public:
    virtual ~SpacedStorage() = default;

    /**
     * Retrieve a pointer to the map representing particular storage space
     * @param space - identifier of required space
     * @return a pointer buffer storage for a space
     */
    virtual std::shared_ptr<BufferBatchableStorage> getSpace(Space space) = 0;

    /**
     * Retrieve a batch to write into the storage atomically
     * @return a new batch
     */
    virtual std::unique_ptr<BufferSpacedBatch> createBatch() = 0;
  };

}  // namespace kagome::storage
