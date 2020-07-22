/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_METADATA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_METADATA_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/metadata.hpp"

namespace kagome::runtime::binaryen {

  class MetadataImpl : public RuntimeApi, public Metadata {
   public:
    explicit MetadataImpl(
        std::shared_ptr<WasmProvider> wasm_provider,
        const std::shared_ptr<RuntimeManager> &runtime_manager);

    ~MetadataImpl() override = default;

    outcome::result<OpaqueMetadata> metadata() override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_METADATA_IMPL_HPP
