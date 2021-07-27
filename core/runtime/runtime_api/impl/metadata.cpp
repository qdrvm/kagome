/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/metadata.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  MetadataImpl::MetadataImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)},
        block_header_repo_{std::move(block_header_repo)} {
    BOOST_ASSERT(executor_);
    BOOST_ASSERT(block_header_repo_);
  }

  outcome::result<Metadata::OpaqueMetadata> MetadataImpl::metadata(
      const boost::optional<primitives::BlockHash> &block_hash) {
    if (block_hash.has_value()) {
      OUTCOME_TRY(header,
                  block_header_repo_->getBlockHeader(block_hash.value()));
      return executor_->callAt<OpaqueMetadata>(
          header.parent_hash,
          "Metadata_metadata");
    }
    return executor_->callAtLatest<OpaqueMetadata>("Metadata_metadata");
  }

}  // namespace kagome::runtime::wavm
