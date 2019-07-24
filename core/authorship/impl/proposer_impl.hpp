/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP
#define KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP

#include "authorship/proposer.hpp"

#include "authorship/block_builder_factory.hpp"
#include "common/logger.hpp"
#include "runtime/block_builder_api.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::authorship {

  class ProposerImpl : public Proposer {
   public:
    ~ProposerImpl() override = default;

    ProposerImpl(
        std::shared_ptr<BlockBuilderFactory> block_builder_factory,
        std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
        std::shared_ptr<runtime::BlockBuilderApi> r_block_builder,
        common::Logger logger = common::createLogger("Proposer"));

    outcome::result<primitives::Block> propose(
        const primitives::BlockId &parent_block_id,
        const primitives::InherentData &inherent_data,
        const primitives::Digest &inherent_digest,
        time::time_t deadline) override;

   private:
    std::shared_ptr<BlockBuilderFactory> block_builder_factory_;
    std::shared_ptr<transaction_pool::TransactionPool> transaction_pool_;
    std::shared_ptr<runtime::BlockBuilderApi> r_block_builder_;
    common::Logger logger_;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP
