/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP

#include "runtime/metadata.hpp"

#include "common/buffer.hpp"
#include "extensions/extension.hpp"

namespace kagome::runtime {

  class RuntimeApi;

  class MetadataImpl : public Metadata {
   public:
    MetadataImpl(common::Buffer state_code,
                 std::shared_ptr<extensions::Extension> extension);

    ~MetadataImpl() override = default;

    outcome::result<OpaqueMetadata> metadata() override;

   private:
    std::unique_ptr<RuntimeApi> runtime_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_METADATA_IMPL_HPP
