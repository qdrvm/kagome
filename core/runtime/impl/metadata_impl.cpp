/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/metadata_impl.hpp"

namespace kagome::runtime {
  using primitives::OpaqueMetadata;

  kagome::runtime::MetadataImpl::MetadataImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::Extension> &extension)
      : RuntimeApi(wasm_provider, extension) {}

  outcome::result<OpaqueMetadata> MetadataImpl::metadata() {
    return execute<OpaqueMetadata>("Metadata_metadata");
  }
}  // namespace kagome::runtime
