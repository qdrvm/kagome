/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/chain_impl.hpp"

#include <utility>

#include <boost/optional/optional_io.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa, ChainImpl::Error, e) {
  using E = kagome::consensus::grandpa::ChainImpl::Error;
  switch (e) {
    case E::BLOCK_AFTER_LIMIT:
      return "target block is after limit";
  }
  return "Unknown error";
}

namespace kagome::consensus::grandpa {

  using primitives::BlockHash;
  using primitives::BlockNumber;

  ChainImpl::ChainImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repository)
      : block_tree_{std::move(block_tree)},
        header_repository_{std::move(header_repository)},
        logger_{common::createLogger("Chain API:")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repository_ != nullptr);
  }

  outcome::result<std::vector<BlockHash>> ChainImpl::getAncestry(
      const BlockHash &base, const BlockHash &block) const {
    OUTCOME_TRY(chain, block_tree_->getChainByBlocks(base, block));
    std::vector<BlockHash> result_chain(chain.size() - 2);
    std::copy(chain.rbegin() + 1, chain.rend() - 1, result_chain.begin());
    return result_chain;
  }

  outcome::result<BlockInfo> ChainImpl::bestChainContaining(
      const BlockHash &base) const {
    boost::optional<uint64_t> limit =
        boost::none;  // TODO(Harrm) authority_set.current_limit
    // find out how to obtain it and whether it is needed

    logger_->debug("Finding best chain containing block {}", base.toHex());
    OUTCOME_TRY(best_info, block_tree_->getBestContaining(base, boost::none));
    auto best_hash = best_info.block_hash;

    if (limit.has_value() && best_info.block_number > limit) {
      logger_->error(
          "Encountered error finding best chain containing {} with limit {}: "
          "target block is after limit",
          best_hash.toHex(),
          limit);
      return Error::BLOCK_AFTER_LIMIT;
    }

    // OUTCOME_TRY(base_header, header_repository_->getBlockHeader(base));
    // auto diff = best_info.block_number - base_header.number;
    // auto target = base_header.number + (diff * 3 + 2) / 4;
    auto target = best_info.block_number;
    target = limit.has_value() ? std::min(limit.value(), target) : target;

    OUTCOME_TRY(best_header, header_repository_->getBlockHeader(best_hash));

    // walk backwards until we find the target block
    while (true) {
      if (best_header.number == target) {
        return BlockInfo{primitives::BlockNumber{best_header.number},
                         best_hash};
      }
      best_hash = best_header.parent_hash;
      OUTCOME_TRY(new_best_header,
                  header_repository_->getBlockHeader(best_hash));
      best_header = new_best_header;
    }
  }
}  // namespace kagome::consensus::grandpa
