/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/environment_impl.hpp"

#include <utility>

#include <boost/optional/optional_io.hpp>
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa,
                            EnvironmentImpl::Error,
                            e) {
  using E = kagome::consensus::grandpa::EnvironmentImpl::Error;
  switch (e) {
    case E::BLOCK_AFTER_LIMIT:
      return "target block is after limit";
  }
  return "Unknown error";
}

namespace kagome::consensus::grandpa {

  using primitives::BlockHash;
  using primitives::BlockNumber;

  EnvironmentImpl::EnvironmentImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
      std::shared_ptr<Gossiper> gossiper)
      : block_tree_{std::move(block_tree)},
        header_repository_{std::move(header_repository)},
        gossiper_{std::move(gossiper)},
        logger_{common::createLogger("Grandpa environment:")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repository_ != nullptr);
    BOOST_ASSERT(gossiper_ != nullptr);
  }

  outcome::result<std::vector<BlockHash>> EnvironmentImpl::getAncestry(
      const BlockHash &base, const BlockHash &block) const {
    OUTCOME_TRY(chain, block_tree_->getChainByBlocks(base, block));
    std::vector<BlockHash> result_chain(chain.size() - 2);
    std::copy(chain.rbegin() + 1, chain.rend() - 1, result_chain.begin());
    return result_chain;
  }

  outcome::result<BlockInfo> EnvironmentImpl::bestChainContaining(
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
        logger_->debug("found best chain: number {}, hash {}",
                       best_header.number,
                       best_hash.toHex());
        return BlockInfo{primitives::BlockNumber{best_header.number},
                         best_hash};
      }
      best_hash = best_header.parent_hash;
      OUTCOME_TRY(new_best_header,
                  header_repository_->getBlockHeader(best_hash));
      best_header = new_best_header;
    }
  }

  outcome::result<void> EnvironmentImpl::proposed(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedPrimaryPropose &propose) {
    VoteMessage message{
        .vote = propose, .round_number = round, .counter = set_id};
    gossiper_->vote(message);
    logger_->info("Primary proposed block with hash {} in grandpa round {}",
                  propose.message.block_hash.toHex(),
                  round);
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::prevoted(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedPrevote &prevote) {
    VoteMessage message{
        .vote = prevote, .round_number = round, .counter = set_id};
    gossiper_->vote(message);
    logger_->info("Prevoted block with hash {} in grandpa round {}",
                  prevote.message.block_hash.toHex(),
                  round);
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::precommitted(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedPrecommit &precommit) {
    VoteMessage message{
        .vote = precommit, .round_number = round, .counter = set_id};
    gossiper_->vote(message);
    logger_->info("Precommitted block with hash {} in grandpa round {}",
                  precommit.message.block_hash.toHex(),
                  round);
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::commit(
      RoundNumber round,
      const BlockInfo &vote,
      const GrandpaJustification &justification) {
    logger_->info("Committed block with hash: {} with number: {}",
                  vote.block_hash,
                  vote.block_number);
    gossiper_->fin(Fin{
        .round_number = round, .vote = vote, .justification = justification});
    return outcome::success();
  }

  void EnvironmentImpl::onCompleted(
      std::function<void(const CompletedRound &)> on_completed_slot) {
    on_completed_.connect(on_completed_slot);
  }

  void EnvironmentImpl::completed(CompletedRound round) {
    BOOST_ASSERT_MSG(
        not on_completed_.empty(),
        "Completed signal in environment cannot be empty when it is invoked");
    on_completed_(round);
  }

  outcome::result<void> EnvironmentImpl::finalize(
      const primitives::BlockHash &block,
      const GrandpaJustification &grandpa_jusitification) {
    primitives::Justification justification;
    justification.data.put(scale::encode(grandpa_jusitification).value());
    auto finalized = block_tree_->finalize(block, justification);
    if (not finalized) {
      logger_->error("Could not finalize block {} from round {} with error: {}",
                     block.toHex(),
                     finalized.error().message());
    }
    logger_->info("Finalized block with hash: {}", block.toHex());
    // TODO(kamilsa): PRE-336 Perform state update

    return finalized;
  }

}  // namespace kagome::consensus::grandpa
