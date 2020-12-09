/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/metadata_impl.hpp"

namespace kagome::runtime::binaryen {
  using primitives::OpaqueMetadata;

  MetadataImpl::MetadataImpl(
      const std::shared_ptr<WasmProvider> &wasm_provider,
      const std::shared_ptr<RuntimeManager> &runtime_manager,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
      : RuntimeApi(wasm_provider, runtime_manager),
        header_repo_(std::move(header_repo)) {
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  outcome::result<OpaqueMetadata> MetadataImpl::metadata(
      const boost::optional<primitives::BlockHash> &block_hash) {
    if (block_hash) {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash.value()));
      return executeAt<OpaqueMetadata>(
          "Metadata_metadata", header.state_root, CallPersistency::EPHEMERAL);
    }
    return execute<OpaqueMetadata>("Metadata_metadata",
                                   CallPersistency::EPHEMERAL);
  }
}  // namespace kagome::runtime::binaryen
