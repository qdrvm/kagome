/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP

#include "runtime/metadata.hpp"

#include "common/buffer.hpp"
#include "runtime/impl/wasm_executor.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime {
  class MetadataImpl : public Metadata {
   public:
    ~MetadataImpl() override = default;

    MetadataImpl(common::Buffer state_code,
                 std::shared_ptr<extensions::Extension> extension);

    outcome::result<OpaqueMetadata> metadata() override;

   private:
    std::shared_ptr<WasmMemory> memory_;
    WasmExecutor executor_;
    common::Buffer state_code_;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP
