/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/metadata.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmMetadata::WavmMetadata(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)},
        block_header_repo_{std::move(block_header_repo)} {
    BOOST_ASSERT(executor_);
    BOOST_ASSERT(block_header_repo_);
  }

  outcome::result<Metadata::OpaqueMetadata> WavmMetadata::metadata(
      const boost::optional<primitives::BlockHash> &block_hash) {
    if (block_hash.has_value()) {
      OUTCOME_TRY(header,
                  block_header_repo_->getBlockHeader(block_hash.value()));
      return executor_->callAt<OpaqueMetadata>(
          header.parent_hash,
          "Metadata_metadata",
          block_hash);
    }
    return executor_->callAtLatest<OpaqueMetadata>("Metadata_metadata",
                                                   block_hash);
  }

}  // namespace kagome::runtime::wavm
