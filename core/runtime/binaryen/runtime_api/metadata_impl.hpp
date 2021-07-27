/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_METADATA_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_METADATA_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/metadata.hpp"

#include "blockchain/block_header_repository.hpp"

namespace kagome::runtime::binaryen {

  class MetadataImpl : public RuntimeApi, public Metadata {
   public:
    explicit MetadataImpl(
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo);

    ~MetadataImpl() override = default;

    outcome::result<OpaqueMetadata> metadata(
        const boost::optional<primitives::BlockHash> &block_hash) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_METADATA_IMPL_HPP
