/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_IMPL_HPP
#define KAGOME_CHAIN_IMPL_HPP

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/grandpa/chain.hpp"

namespace kagome::consensus::grandpa {

  class ChainImpl : public Chain {
   public:
    enum class Error { BLOCK_AFTER_LIMIT = 1 };

    ChainImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repository);
    ~ChainImpl() override = default;

    outcome::result<std::vector<primitives::BlockHash>> getAncestry(
        const primitives::BlockHash &base,
        const primitives::BlockHash &block) const override;

    outcome::result<BlockInfo> bestChainContaining(
        const primitives::BlockHash &base) const override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repository_;
    common::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, ChainImpl::Error);

#endif  // KAGOME_CHAIN_IMPL_HPP
