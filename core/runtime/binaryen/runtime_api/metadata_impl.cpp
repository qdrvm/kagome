/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/metadata_impl.hpp"

namespace kagome::runtime::binaryen {
  using primitives::OpaqueMetadata;

  MetadataImpl::MetadataImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::ExtensionFactory> &extension_factory)
      : RuntimeApi(wasm_provider, extension_factory) {}

  outcome::result<OpaqueMetadata> MetadataImpl::metadata() {
    return execute<OpaqueMetadata>("Metadata_metadata");
  }
}  // namespace kagome::runtime::binaryen
