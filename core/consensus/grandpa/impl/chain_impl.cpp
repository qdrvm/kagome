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
    auto best_info = block_tree_->deepestLeaf();

    if (limit.has_value() && best_info.block_number > limit) {
      return Error::BLOCK_AFTER_LIMIT;
    }

    OUTCOME_TRY(base_header, header_repository_->getBlockHeader(base.unwrap()));
    auto diff = best_info.block_number - base_header.number;
    auto target = base_header.number + (diff * 3 + 2) / 4;
    target = limit.has_value() ? std::min(limit.value(), target) : target;

    // walk backwards until we find the target block
    while(true) {
        if (best_info.block_number == target) {
          return best_info;
        }
        auto best_header = header_repository_->getBlockHeader(best_info.block_number).value().parent_hash;
        auto best_hash = ;
        best_info.block_number =
    }

  }
}  // namespace kagome::consensus::grandpa
