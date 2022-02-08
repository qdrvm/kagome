/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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

  outcome::result<std::unique_ptr<BlockBuilder>> BlockBuilderFactoryImpl::make(
      const kagome::primitives::BlockInfo &parent,
      primitives::Digest inherent_digest) const {
    OUTCOME_TRY(parent_number, header_backend_->getNumberById(parent.hash));
    BOOST_ASSERT(parent_number == parent.number);

    auto number = parent.number + 1;
    primitives::BlockHeader header;
    header.number = number;
    header.parent_hash = parent.hash;
    header.digest = std::move(inherent_digest);

    if (auto res = r_core_->initialize_block(header); not res) {
      logger_->error("Core_initialize_block failed: {}", res.error().message());
      return res.error();
    } else {
      return std::make_unique<BlockBuilderImpl>(
          header, res.value(), r_block_builder_);
    }
  }

}  // namespace kagome::authorship
