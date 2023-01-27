/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SPACED_STORAGE_HPP
#define KAGOME_SPACED_STORAGE_HPP

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
    virtual std::shared_ptr<BufferStorage> getSpace(Space space) = 0;
  };

}  // namespace kagome::storage

#endif  // KAGOME_SPACED_STORAGE_HPP
