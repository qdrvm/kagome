/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_METADATA_HPP
#define KAGOME_CORE_RUNTIME_IMPL_METADATA_HPP

#include "runtime/runtime_api/metadata.hpp"

#include "blockchain/block_header_repository.hpp"
#include "common/lru_cache.hpp"

namespace kagome::runtime {

  class Executor;

  class MetadataImpl final : public Metadata {
   public:
    MetadataImpl(std::shared_ptr<Executor> executor);

    outcome::result<OpaqueMetadata> metadata(
        const primitives::BlockHash &block_hash) override;

   private:
    std::shared_ptr<Executor> executor_;

    LruCache<primitives::BlockHash, OpaqueMetadata, true> metadata_{10};
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_METADATA_HPP
