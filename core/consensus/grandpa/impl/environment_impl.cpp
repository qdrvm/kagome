/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/environment_impl.hpp"

#include <boost/optional/optional_io.hpp>
#include <utility>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "consensus/grandpa/justification_observer.hpp"
#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "crypto/hasher.hpp"
#include "dispute_coordinator/dispute_coordinator.hpp"
#include "dispute_coordinator/types.hpp"
#include "network/grandpa_transmitter.hpp"
#include "parachain/backing/store.hpp"
#include "primitives/common.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  using primitives::BlockHash;
  using primitives::BlockNumber;
  using primitives::Justification;

  EnvironmentImpl::EnvironmentImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
      std::shared_ptr<AuthorityManager> authority_manager,
      std::shared_ptr<network::GrandpaTransmitter> transmitter,
      std::shared_ptr<parachain::IApprovedAncestor> approved_ancestor,
      LazySPtr<JustificationObserver> justification_observer,
      std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<parachain::BackingStore> backing_store,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<boost::asio::io_context> main_thread_context)
      : block_tree_{std::move(block_tree)},
        header_repository_{std::move(header_repository)},
        authority_manager_{std::move(authority_manager)},
        transmitter_{std::move(transmitter)},
        approved_ancestor_(std::move(approved_ancestor)),
        justification_observer_(std::move(justification_observer)),
        dispute_coordinator_(std::move(dispute_coordinator)),
        parachain_api_(std::move(parachain_api)),
        backing_store_(std::move(backing_store)),
        hasher_(std::move(hasher)),
        main_thread_context_{std::move(main_thread_context)},
        logger_{log::createLogger("GrandpaEnvironment", "grandpa")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repository_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);
    BOOST_ASSERT(transmitter_ != nullptr);
    BOOST_ASSERT(dispute_coordinator_ != nullptr);
    BOOST_ASSERT(parachain_api_ != nullptr);
    BOOST_ASSERT(backing_store_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    auto kApprovalLag = "kagome_parachain_approval_checking_finality_lag";
    metrics_registry_->registerGaugeFamily(
        kApprovalLag,
        "How far behind the head of the chain the Approval Checking protocol "
        "wants to vote");
    metric_approval_lag_ = metrics_registry_->registerGaugeMetric(kApprovalLag);

    main_thread_context_.start();
  }

  outcome::result<bool> EnvironmentImpl::hasBlock(
      const primitives::BlockHash &block) const {
    return block_tree_->hasBlockHeader(block);
  }

  outcome::result<std::vector<BlockHash>> EnvironmentImpl::getAncestry(
      const BlockHash &base, const BlockHash &block) const {
    // if base equal to block, then return list with single block
    if (base == block) {
      return std::vector<BlockHash>{base};
    }

    OUTCOME_TRY(chain, block_tree_->getChainByBlocks(base, block));
    std::reverse(chain.begin(), chain.end());
    return chain;
  }

  bool EnvironmentImpl::hasAncestry(const BlockHash &base,
                                    const BlockHash &block) const {
    return block_tree_->hasDirectChain(base, block);
  }

  outcome::result<BlockInfo> EnvironmentImpl::bestChainContaining(
      const BlockHash &base_hash,
      std::optional<VoterSetId> voter_set_id) const {
    SL_DEBUG(logger_, "Finding best chain containing block {}", base_hash);

    OUTCOME_TRY(best_block,
                block_tree_->getBestContaining(base_hash, std::nullopt));

    // Must finalize block with scheduled/forced change digest first
    auto finalized = block_tree_->getLastFinalized();

    auto approved = approved_ancestor_->approvedAncestor(finalized, best_block);

    auto lag = best_block.number - approved.number;

    if (best_block.number > approved.number) {
      SL_INFO(logger_,
              "Found best chain is longer than approved: {} > {}; truncate it",
              best_block,
              approved);
      best_block = approved;
      metric_approval_lag_->set(lag);
    } else {
      metric_approval_lag_->set(0);
    }

    OUTCOME_TRY(best_chain,
                block_tree_->getChainByBlocks(finalized.hash, best_block.hash));

    std::vector<dispute::BlockDescription> block_descriptions;

    primitives::BlockHash parent_hash;
    for (auto &block_hash : best_chain) {
      // Skip base
      if (block_hash == finalized.hash) {
        parent_hash = block_hash;
        continue;
      }

      auto session_index_res =
          parachain_api_->session_index_for_child(parent_hash);
      if (session_index_res.has_error()) {
        SL_WARN(logger_,
                "Unable to query undisputed chain, "
                "'cause can't get session index for one best chain block: {}",
                session_index_res.error());
        return session_index_res.as_failure();
      }
      const auto &session_index = session_index_res.value();

      auto candidates_for_block = backing_store_->get(block_hash);

      std::vector<dispute::CandidateHash> candidates;

      for (auto &candidate : candidates_for_block) {
        network::CandidateReceipt receipt;
        receipt.descriptor = candidate.candidate.descriptor,
        receipt.commitments_hash = hasher_->blake2b_256(
            scale::encode(candidate.candidate.commitments).value());

        auto candidate_hash = hasher_->blake2b_256(scale::encode().value());

        candidates.push_back(candidate_hash);
      }

      block_descriptions.emplace_back(
          dispute::BlockDescription{.block_hash = block_hash,
                                    .session = session_index,
                                    .candidates = std::move(candidates)});

      parent_hash = block_hash;
    }

    auto promise_res = std::promise<outcome::result<primitives::BlockInfo>>();
    auto res_future = promise_res.get_future();

    dispute_coordinator_->determineUndisputedChain(
        finalized,
        block_descriptions,
        [promise_res = std::ref(promise_res)](
            outcome::result<primitives::BlockInfo> res) {
          promise_res.get().set_value(std::move(res));
        });

    auto best_undisputed_block_res = res_future.get();
    if (best_undisputed_block_res.has_error()) {
      SL_WARN(logger_,
              "Unable to query undisputed chain: {}",
              best_undisputed_block_res.error());
      return best_undisputed_block_res.as_failure();
    }

    const auto &best_undisputed_block = best_undisputed_block_res.value();

    best_block = best_undisputed_block;
    auto block = best_block;
    while (block.number > finalized.number) {
      OUTCOME_TRY(header, header_repository_->getBlockHeader(block.hash));
      if (HasAuthoritySetChange{header}) {
        best_block = block;
      }
      block = *header.parentInfo();
    }

    // Select best block with actual set_id
    if (voter_set_id.has_value()) {
      while (best_block.number > finalized.number) {
        OUTCOME_TRY(header,
                    header_repository_->getBlockHeader(best_block.hash));
        auto parent_block = *header.parentInfo();

        auto voter_set = authority_manager_->authorities(
            parent_block, IsBlockFinalized{true});

        if (voter_set.has_value()
            and voter_set.value()->id <= voter_set_id.value()) {
          // found
          break;
        }

        best_block = parent_block;
      }
    }

    SL_DEBUG(logger_, "Found best chain: {}", best_block);
    return best_block;
  }

  void EnvironmentImpl::onCatchUpRequested(const libp2p::peer::PeerId &peer_id,
                                           VoterSetId set_id,
                                           RoundNumber round_number) {
    REINVOKE(main_thread_context_,
             onCatchUpRequested,
             peer_id,
             set_id,
             round_number);
    network::CatchUpRequest message{.round_number = round_number,
                                    .voter_set_id = set_id};
    transmitter_->sendCatchUpRequest(peer_id, std::move(message));
  }

  void EnvironmentImpl::onCatchUpRespond(
      const libp2p::peer::PeerId &peer_id,
      VoterSetId set_id,
      RoundNumber round_number,
      std::vector<SignedPrevote> prevote_justification,
      std::vector<SignedPrecommit> precommit_justification,
      BlockInfo best_final_candidate) {
    REINVOKE(main_thread_context_,
             onCatchUpRespond,
             peer_id,
             set_id,
             round_number,
             std::move(prevote_justification),
             std::move(precommit_justification),
             best_final_candidate);
    SL_DEBUG(logger_, "Send Catch-Up-Response upto round {}", round_number);
    network::CatchUpResponse message{
        .voter_set_id = set_id,
        .round_number = round_number,
        .prevote_justification = std::move(prevote_justification),
        .precommit_justification = std::move(precommit_justification),
        .best_final_candidate = best_final_candidate};
    transmitter_->sendCatchUpResponse(peer_id, std::move(message));
  }

  void EnvironmentImpl::onVoted(RoundNumber round,
                                VoterSetId set_id,
                                const SignedMessage &vote) {
    main_thread_context_.execute([wself{weak_from_this()},
                                  round{std::move(round)},
                                  set_id{std::move(set_id)},
                                  vote]() mutable {
      if (auto self = wself.lock()) {
        SL_VERBOSE(
            self->logger_,
            "Round #{}: Send {} signed by {} for block {}",
            round,
            visit_in_place(
                vote.message,
                [&](const Prevote &) { return "prevote"; },
                [&](const Precommit &) { return "precommit"; },
                [&](const PrimaryPropose &) { return "primary propose"; }),
            vote.id,
            vote.getBlockInfo());

        self->transmitter_->sendVoteMessage(
            network::GrandpaVote{{.round_number = std::move(round),
                                  .counter = std::move(set_id),
                                  .vote = std::move(vote)}});
      }
    });
  }

  void EnvironmentImpl::sendState(const libp2p::peer::PeerId &peer_id,
                                  const MovableRoundState &state,
                                  VoterSetId voter_set_id) {
    main_thread_context_.execute([wself{weak_from_this()},
                                  peer_id,
                                  voter_set_id{std::move(voter_set_id)},
                                  state]() mutable {
      if (auto self = wself.lock()) {
        auto send = [&](const SignedMessage &vote) {
          SL_DEBUG(
              self->logger_,
              "Round #{}: Send {} signed by {} for block {} (as send state)",
              state.round_number,
              visit_in_place(
                  vote.message,
                  [&](const Prevote &) { return "prevote"; },
                  [&](const Precommit &) { return "precommit"; },
                  [&](const PrimaryPropose &) { return "primary propose"; }),
              vote.id,
              vote.getBlockInfo());

          ;
          self->transmitter_->sendVoteMessage(
              peer_id,
              network::GrandpaVote{{.round_number = state.round_number,
                                    .counter = voter_set_id,
                                    .vote = vote}});
        };

        for (const auto &vv : state.votes) {
          visit_in_place(
              vv,
              [&](const SignedMessage &vote) { send(vote); },
              [&](const EquivocatorySignedMessage &pair_vote) {
                send(pair_vote.first);
                send(pair_vote.second);
              });
        }
      }
    });
  }

  void EnvironmentImpl::onCommitted(RoundNumber round,
                                    VoterSetId voter_ser_id,
                                    const BlockInfo &vote,
                                    const GrandpaJustification &justification) {
    if (round == 0) {
      return;
    }

    REINVOKE(main_thread_context_,
             onCommitted,
             round,
             voter_ser_id,
             vote,
             justification);
    SL_DEBUG(logger_, "Round #{}: Send commit of block {}", round, vote);

    network::FullCommitMessage message{
        .round = round,
        .set_id = voter_ser_id,
        .message = {.target_hash = vote.hash, .target_number = vote.number}};
    for (const auto &item : justification.items) {
      BOOST_ASSERT(item.is<Precommit>());
      const auto &precommit = boost::relaxed_get<Precommit>(item.message);
      message.message.precommits.push_back(precommit);
      message.message.auth_data.emplace_back(item.signature, item.id);
    }
    transmitter_->sendCommitMessage(std::move(message));
  }

  void EnvironmentImpl::onNeighborMessageSent(RoundNumber round,
                                              VoterSetId set_id,
                                              BlockNumber last_finalized) {
    REINVOKE(main_thread_context_,
             onNeighborMessageSent,
             round,
             set_id,
             last_finalized);
    SL_DEBUG(logger_, "Round #{}: Send neighbor message", round);

    network::GrandpaNeighborMessage message{.round_number = round,
                                            .voter_set_id = set_id,
                                            .last_finalized = last_finalized};
    transmitter_->sendNeighborMessage(std::move(message));
  }

  void EnvironmentImpl::applyJustification(
      const BlockInfo &block_info,
      const primitives::Justification &raw_justification,
      ApplyJustificationCb &&cb) {
    auto res = scale::decode<GrandpaJustification>(raw_justification.data);
    if (res.has_error()) {
      cb(res.as_failure());
      return;
    }
    auto &&justification = std::move(res.value());

    if (justification.block_info != block_info) {
      cb(VotingRoundError::JUSTIFICATION_FOR_WRONG_BLOCK);
      return;
    }

    SL_DEBUG(logger_,
             "Trying to apply justification on round #{} for block {}",
             justification.round_number,
             justification.block_info);

    justification_observer_.get()->applyJustification(justification,
                                                      std::move(cb));
  }

  outcome::result<void> EnvironmentImpl::finalize(
      VoterSetId id, const GrandpaJustification &grandpa_justification) {
    primitives::Justification justification;
    OUTCOME_TRY(enc, scale::encode(grandpa_justification));
    justification.data.put(enc);
    OUTCOME_TRY(block_tree_->finalize(grandpa_justification.block_info.hash,
                                      justification));

    return outcome::success();
  }

  outcome::result<GrandpaJustification> EnvironmentImpl::getJustification(
      const BlockHash &block_hash) {
    OUTCOME_TRY(encoded_justification,
                block_tree_->getBlockJustification(block_hash));

    OUTCOME_TRY(
        grandpa_justification,
        scale::decode<GrandpaJustification>(encoded_justification.data));

    return outcome::success(std::move(grandpa_justification));
  }

}  // namespace kagome::consensus::grandpa
