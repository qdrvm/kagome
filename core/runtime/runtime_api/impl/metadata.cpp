/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/metadata.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  MetadataImpl::MetadataImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<Metadata::OpaqueMetadata> MetadataImpl::metadata(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(ref, metadata_.get_else(block_hash, [&] {
      return executor_->callAt<OpaqueMetadata>(block_hash, "Metadata_metadata");
    }));
    return *ref;
  }

}  // namespace kagome::runtime
