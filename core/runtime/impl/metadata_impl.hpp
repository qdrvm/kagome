/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/impl/runtime_api.hpp"
#include "runtime/metadata.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime {
  class MetadataImpl : public RuntimeApi, public Metadata {
   public:
    MetadataImpl(const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
                 const std::shared_ptr<extensions::Extension> &extension);

    ~MetadataImpl() override = default;

    outcome::result<OpaqueMetadata> metadata() override;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP
