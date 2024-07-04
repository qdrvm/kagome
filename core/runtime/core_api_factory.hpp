/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "common/buffer_view.hpp"
#include "outcome/outcome.hpp"

namespace kagome::runtime {
  class RestrictedCore;
  class TrieStorageProvider;

  /**
   * A factory for Core API, used where an isolated runtime environment
   * is required
   */
  class CoreApiFactory {
   public:
    virtual ~CoreApiFactory() = default;

    [[nodiscard]] virtual outcome::result<std::unique_ptr<RestrictedCore>> make(
        BufferView code,
        std::shared_ptr<TrieStorageProvider> storage_provider) const = 0;
  };

}  // namespace kagome::runtime
