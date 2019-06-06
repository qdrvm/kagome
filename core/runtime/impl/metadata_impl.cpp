/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/metadata_impl.hpp"

#include "scale/scale.hpp"
#include "scale/scale_error.hpp"

namespace kagome::runtime {
  using OpaqueMetadata = primitives::OpaqueMetadata;

  MetadataImpl::MetadataImpl(common::Buffer state_code,
                             std::shared_ptr<extensions::Extension> extension)
      : memory_(extension->memory()),
        executor_(std::move(extension)),
        state_code_(std::move(state_code)) {}

  outcome::result<std::vector<uint8_t>> MetadataImpl::metadata() {
    wasm::LiteralList ll{wasm::Literal(0u), wasm::Literal(0u)};

    OUTCOME_TRY(res, executor_.call(state_code_, "Metadata_metadata", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    return scale::decode<OpaqueMetadata>(buffer);
  }
}  // namespace kagome::runtime
