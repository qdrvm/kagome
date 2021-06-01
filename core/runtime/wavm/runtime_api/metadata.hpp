/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_METADATA_HPP
#define KAGOME_CORE_RUNTIME_WAVM_METADATA_HPP

#include "runtime/metadata.hpp"

#include "blockchain/block_header_repository.hpp"

namespace kagome::runtime::wavm {

  class Executor;

  class WavmMetadata final : public Metadata {
   public:
    WavmMetadata(
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
        std::shared_ptr<Executor> executor);

    outcome::result<OpaqueMetadata> metadata(
        const boost::optional<primitives::BlockHash> &block_hash) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_METADATA_HPP
