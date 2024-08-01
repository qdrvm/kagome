/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_factory_impl.hpp"

#include "authorship/impl/block_builder_impl.hpp"

namespace kagome::authorship {

  BlockBuilderFactoryImpl::BlockBuilderFactoryImpl(
      std::shared_ptr<runtime::Core> r_core,
      std::shared_ptr<runtime::BlockBuilder> r_block_builder,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_backend)
      : r_core_(std::move(r_core)),
        r_block_builder_(std::move(r_block_builder)),
        header_backend_(std::move(header_backend)),
        logger_{log::createLogger("BlockBuilderFactory", "authorship")} {
    BOOST_ASSERT(r_core_ != nullptr);
    BOOST_ASSERT(r_block_builder_ != nullptr);
    BOOST_ASSERT(header_backend_ != nullptr);
  }

  BlockBuilderFactory::Result BlockBuilderFactoryImpl::make(
      const kagome::primitives::BlockInfo &parent,
      primitives::Digest inherent_digest,
      TrieChangesTrackerOpt changes_tracker) const {
#ifndef BOOST_ASSERT_IS_VOID
    OUTCOME_TRY(parent_number, header_backend_->getNumberById(parent.hash));
#endif
    BOOST_ASSERT(parent_number == parent.number);

    // Create a new block header with the incremented number, parent's hash, and
    // the inherent digest
    auto number = parent.number + 1;
    primitives::BlockHeader header{
        .number = number,
        .parent_hash = parent.hash,
        .digest = std::move(inherent_digest),
    };

    // Try to initialize the block with the created header and changes tracker
    if (auto res =
            r_core_->initialize_block(header, std::move(changes_tracker));
        not res) {
      logger_->error("Core_initialize_block failed: {}", res.error());
      return res.error();
    } else {
      auto &[ctx, mode] = res.value();
      return std::make_pair(std::make_unique<BlockBuilderImpl>(
                                header, std::move(ctx), r_block_builder_),
                            mode);
    }
  }

}  // namespace kagome::authorship
