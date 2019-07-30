/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_factory_impl.hpp"

#include "authorship/impl/block_builder_impl.hpp"

namespace kagome::authorship {

  BlockBuilderFactoryImpl::BlockBuilderFactoryImpl(
      std::shared_ptr<runtime::Core> r_core,
      std::shared_ptr<runtime::BlockBuilderApi> r_block_builder,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_backend)
      : r_core_(std::move(r_core)),
        r_block_builder_(std::move(r_block_builder)),
        header_backend_(std::move(header_backend)) {
    BOOST_ASSERT(r_core_ != nullptr);
    BOOST_ASSERT(r_block_builder_ != nullptr);
    BOOST_ASSERT(header_backend_ != nullptr);
  }

  outcome::result<std::unique_ptr<BlockBuilder>>
  BlockBuilderFactoryImpl::create(
      const kagome::primitives::BlockId &parent_id,
      const kagome::primitives::Digest &inherent_digest) const {
    // based on
    // https://github.com/paritytech/substrate/blob/dbf322620948935d2bbae214504e6c668c3073ed/core/basic-authorship/src/basic_authorship.rs#L94

    OUTCOME_TRY(parent_hash, header_backend_->getHashById(parent_id));
    OUTCOME_TRY(parent_number, header_backend_->getNumberById(parent_id));

    primitives::BlockHeader parent_header;
    parent_header.number = parent_number;
    parent_header.parent_hash = parent_hash;
    parent_header.digests = inherent_digest;

    OUTCOME_TRY(r_core_->initialise_block(parent_header));

    return std::make_unique<BlockBuilderImpl>(std::move(parent_header),
                                              r_block_builder_);
  }

}  // namespace kagome::authorship
