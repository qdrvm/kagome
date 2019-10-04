/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/chain_impl.hpp"

namespace kagome::consensus::grandpa {

  outcome::result<std::vector<BlockHash>> ChainImpl::ancestry(BlockHash base,
                                                              BlockHash block) {
    OUTCOME_TRY(chain,
                block_tree_->getChainByBlocks(block.unwrap(), base.unwrap()));
    std::vector<BlockHash> ancestry(chain.size());
    std::transform(chain.begin(), chain.end(), ancestry.begin(), [](auto &&h) {
      return BlockHash{h};
    });
    return ancestry;
  }

  outcome::result<BlockInfo> ChainImpl::bestChainContaining(BlockHash base) {
    auto limit = boost::make_optional(
        24ul);  // TODO(Harrm) authority_set.current_limit - find
                // out how to obtain it and whether it is needed
    OUTCOME_TRY(best_info,
                block_tree_->getBestContaining(base.unwrap(), boost::none));
    auto best_hash = best_info.block_hash;

    if (limit.has_value() && best_info.block_number > limit) {
      return Error::BLOCK_AFTER_LIMIT;
    }

    OUTCOME_TRY(base_header, header_repository_->getBlockHeader(base.unwrap()));
    auto diff = best_info.block_number - base_header.number;
    auto target = base_header.number + (diff * 3 + 2) / 4;
    OUTCOME_TRY(best_header, header_repository_->getBlockHeader(best_hash));
    target = limit.has_value() ? std::min(limit.value(), target) : target;

    // walk backwards until we find the target block
    while (true) {
      if (best_info.block_number == target) {
        return BlockInfo{BlockNumber{best_info.block_number},
                         best_hash};
      }
      best_hash = best_header.parent_hash;
      OUTCOME_TRY(best_header, header_repository_->getBlockHeader(best_hash));
    }
  }
}  // namespace kagome::consensus::grandpa
