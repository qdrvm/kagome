/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_METADATA_HPP
#define KAGOME_CORE_RUNTIME_WAVM_METADATA_HPP

#include "runtime/metadata.hpp"

#include "blockchain/block_header_repository.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmMetadata final : public Metadata {
   public:
    WavmMetadata(
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
        std::shared_ptr<Executor> executor)
        : executor_{std::move(executor)},
          block_header_repo_{std::move(block_header_repo)} {
      BOOST_ASSERT(executor_);
      BOOST_ASSERT(block_header_repo_);
    }

    outcome::result<OpaqueMetadata> metadata(
        const boost::optional<primitives::BlockHash> &block_hash) override {
      if (block_hash.has_value()) {
        OUTCOME_TRY(header,
                    block_header_repo_->getBlockHeader(block_hash.value()));
        return executor_->callAt<OpaqueMetadata>(
            header.state_root, "Metadata_metadata", block_hash);
      }
      return executor_->callAtLatest<OpaqueMetadata>("Metadata_metadata",
                                                     block_hash);
    }

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_METADATA_HPP
