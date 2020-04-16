/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP
#define KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP

#include "authorship/proposer.hpp"

#include "authorship/block_builder_factory.hpp"
#include "common/logger.hpp"
#include "runtime/block_builder.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::authorship {

  class ProposerImpl : public Proposer {
   public:
    ~ProposerImpl() override = default;

    ProposerImpl(
        std::shared_ptr<BlockBuilderFactory> block_builder_factory,
        std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
        std::shared_ptr<runtime::BlockBuilder> r_block_builder);

    outcome::result<primitives::Block> propose(
        const primitives::BlockId &parent_block_id,
        const primitives::InherentData &inherent_data,
        const primitives::Digest &inherent_digest) override;

   private:
    std::shared_ptr<BlockBuilderFactory> block_builder_factory_;
    std::shared_ptr<transaction_pool::TransactionPool> transaction_pool_;
    std::shared_ptr<runtime::BlockBuilder> r_block_builder_;
    common::Logger logger_ = common::createLogger("Proposer");
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP
