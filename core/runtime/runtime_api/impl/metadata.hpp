/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/metadata.hpp"

#include "runtime/runtime_api/impl/lru.hpp"

namespace kagome::runtime {

  class Executor;

  class MetadataImpl final : public Metadata {
   public:
    MetadataImpl(
        std::shared_ptr<Executor> executor,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker);

    outcome::result<OpaqueMetadata> metadata(
        const primitives::BlockHash &block_hash) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker_;

    RuntimeApiLruCode<OpaqueMetadata> metadata_{10};
  };

}  // namespace kagome::runtime
