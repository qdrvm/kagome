/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/metadata_impl.hpp"

#include "scale/scale.hpp"
#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  using primitives::OpaqueMetadata;

  kagome::runtime::MetadataImpl::MetadataImpl(
      common::Buffer state_code,
      std::shared_ptr<extensions::Extension> extension) {
    executor_ = std::make_unique<RuntimeApi>(std::move(state_code),
                                                  std::move(extension));
  }

  outcome::result<OpaqueMetadata> MetadataImpl::metadata() {
    return TypedExecutor<OpaqueMetadata>(executor_.get())
        .execute("Metadata_metadata");
  }
}  // namespace kagome::runtime
