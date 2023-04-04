/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP
#define KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP

#include "authorship/block_builder_factory.hpp"

#include "blockchain/block_header_repository.hpp"
#include "log/logger.hpp"

namespace kagome::authorship {

  class BlockBuilderFactoryImpl : public BlockBuilderFactory {
   public:
    ~BlockBuilderFactoryImpl() override = default;

    BlockBuilderFactoryImpl(
        std::shared_ptr<runtime::Core> r_core,
        std::shared_ptr<runtime::BlockBuilder> r_block_builder,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_backend);

    outcome::result<std::unique_ptr<BlockBuilder>> make(
        const kagome::primitives::BlockInfo &parent_block,
        primitives::Digest inherent_digest,
        TrieChangesTrackerOpt changes_tracker) const override;

   private:
    std::shared_ptr<runtime::Core> r_core_;
    std::shared_ptr<runtime::BlockBuilder> r_block_builder_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_backend_;
    log::Logger logger_;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_FACTORY_IMPL_HPP
