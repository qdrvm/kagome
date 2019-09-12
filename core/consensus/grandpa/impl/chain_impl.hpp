/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_IMPL_HPP
#define KAGOME_CHAIN_IMPL_HPP

#include "consensus/grandpa/chain.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/block_header_repository.hpp"

namespace kagome::consensus::grandpa {

  class ChainImpl : public Chain {
   public:
    enum class Error {
      BLOCK_AFTER_LIMIT = 1
    };

    ~ChainImpl() override = default;

    outcome::result<std::vector<BlockHash>> ancestry(BlockHash base,
                                                     BlockHash block) override;

    outcome::result<BlockInfo> bestChainContaining(BlockHash base) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repository_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CHAIN_IMPL_HPP
