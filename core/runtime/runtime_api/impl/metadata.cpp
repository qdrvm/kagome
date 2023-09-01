/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/metadata.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  MetadataImpl::MetadataImpl(
      std::shared_ptr<Executor> executor,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker)
      : executor_{std::move(executor)},
        header_repo_{std::move(header_repo)},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<Metadata::OpaqueMetadata> MetadataImpl::metadata(
      const primitives::BlockHash &block_hash) {
    return metadata_.call(*header_repo_,
                          *runtime_upgrade_tracker_,
                          *executor_,
                          block_hash,
                          "Metadata_metadata");
  }

}  // namespace kagome::runtime
