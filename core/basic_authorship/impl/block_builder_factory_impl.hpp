/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BASIC_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP
#define KAGOME_CORE_BASIC_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP

#include "basic_authorship/block_builder_factory.hpp"

#include "blockchain/block_header_repository.hpp"

namespace kagome::basic_authorship {

  class BlockBuilderFactoryImpl : public BlockBuilderFactory {
   public:
    ~BlockBuilderFactoryImpl() override = default;

    BlockBuilderFactoryImpl(
        std::shared_ptr<runtime::Core> r_core,
        std::shared_ptr<runtime::BlockBuilderApi> r_block_builder,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_backend);

    outcome::result<std::unique_ptr<BlockBuilder>> create(
        const kagome::primitives::BlockId &parent_id,
        const kagome::primitives::Digest &inherent_digest) const override;

   private:
    std::shared_ptr<runtime::Core> r_core_;
    std::shared_ptr<runtime::BlockBuilderApi> r_block_builder_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_backend_;
  };

}  // namespace kagome::basic_authorship

#endif  // KAGOME_CORE_BASIC_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP
