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
      const primitives::BlockHash &block_hash) {
    return executor_->callAt<OpaqueMetadata>(block_hash, "Metadata_metadata");
  }

}  // namespace kagome::runtime
