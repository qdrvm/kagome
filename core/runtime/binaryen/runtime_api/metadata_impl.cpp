/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/metadata_impl.hpp"

namespace kagome::runtime::binaryen {
  using primitives::OpaqueMetadata;

  MetadataImpl::MetadataImpl(
      std::shared_ptr<WasmProvider> wasm_provider,
      const std::shared_ptr<RuntimeManager> &runtime_manager)
      : RuntimeApi(std::move(wasm_provider), runtime_manager) {}

  outcome::result<OpaqueMetadata> MetadataImpl::metadata() {
    return execute<OpaqueMetadata>("Metadata_metadata",
                                   CallPersistency::EPHEMERAL);
  }
}  // namespace kagome::runtime::binaryen
