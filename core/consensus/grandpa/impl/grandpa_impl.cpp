/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/grandpa_impl.hpp"

#include <boost/operators.hpp>

#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "scale/scale.hpp"
#include "storage/database_error.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::consensus::grandpa {

  // help struct to correctly compare rounds in different voter sets
  // TODO(a.krutikov): refactor using concepts
  struct FullRound : boost::less_than_comparable<FullRound>,
                     boost::equality_comparable<FullRound> {
    MembershipCounter v_id;
    RoundNumber r_no;
    explicit FullRound(const std::shared_ptr<const VotingRound> &round)
        : v_id(round->voterSetId()), r_no(round->roundNumber()) {}
    explicit FullRound(const network::GrandpaNeighborMessage &msg)
        : v_id(msg.voter_set_id), r_no(msg.round_number) {}
    explicit FullRound(const network::CatchUpRequest &msg)
        : v_id(msg.voter_set_id), r_no(msg.round_number) {}
    explicit FullRound(const network::CatchUpResponse &msg)
        : v_id(msg.voter_set_id), r_no(msg.round_number) {}
    explicit FullRound(const VoteMessage &msg)
        : v_id(msg.counter), r_no(msg.round_number) {}

    operator network::CatchUpRequest() const {
      return network::CatchUpRequest{r_no, v_id};
    }

    bool operator<(const FullRound &round) const {
      return v_id == round.v_id ? r_no < round.r_no : v_id < round.v_id;
    }

    bool operator==(const FullRound &round) const {
      return v_id == round.v_id && r_no == round.r_no;
    }
  };

  GrandpaImpl::GrandpaImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<Environment> environment,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      const std::shared_ptr<crypto::Ed25519Keypair> &keypair,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<authority::AuthorityManager> authority_manager)
      : app_state_manager_(std::move(app_state_manager)),
        environment_{std::move(environment)},
        storage_{std::move(storage)},
        crypto_provider_{std::move(crypto_provider)},
        grandpa_api_{std::move(grandpa_api)},
        keypair_{keypair},
        clock_{std::move(clock)},
        io_context_{std::move(io_context)},
        authority_manager_(std::move(authority_manager)) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(environment_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(crypto_provider_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(io_context_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);

    app_state_manager_->takeControl(*this);
  }

  bool GrandpaImpl::prepare() {
    // Set themselves in environment
    environment_->setJustificationObserver(shared_from_this());

    // Lambda which is executed when voting round is completed.
    environment_->doOnCompleted(
        [wp = weak_from_this()](
            outcome::result<MovableRoundState> completed_round_res) {
          if (auto self = wp.lock()) {
            self->onCompletedRound(std::move(completed_round_res));
          }
        });
    return true;
  }

  bool GrandpaImpl::start() {
    // Obtain last completed round
    auto round_state_res = getLastCompletedRound();
    if (not round_state_res.has_value()) {
      logger_->critical(
          "Can't retrieve last round data: {}. Stopping grandpa execution",
          round_state_res.error().message());
      return false;
    }
    auto &round_state = round_state_res.value();

    SL_DEBUG(logger_,
             "Grandpa will be started with round #{}",
             round_state.round_number + 1);

    auto authorities_res =
        authority_manager_->authorities(round_state.last_finalized_block, true);
    if (not authorities_res.has_value()) {
      logger_->critical(
          "Can't retrieve authorities: {}. Stopping grandpa execution",
          authorities_res.error().message());
      return false;
    }
    auto &authorities = authorities_res.value();

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                     authority.weight);
    }

    current_round_ = makeInitialRound(round_state, std::move(voters));
    if (current_round_ == nullptr) {
      logger_->critical(
          "Next round hasn't been made. Stopping grandpa execution");
      return false;
    }

    BOOST_ASSERT(current_round_->finalizable());
    BOOST_ASSERT(current_round_->finalizedBlock() == round_state.finalized);

    executeNextRound();

    if (not current_round_) {
      return false;
    }

    return true;
  }

  void GrandpaImpl::stop() {}

  std::shared_ptr<VotingRound> GrandpaImpl::makeInitialRound(
      const MovableRoundState &round_state, std::shared_ptr<VoterSet> voters) {
    auto vote_graph = std::make_shared<VoteGraphImpl>(
        round_state.last_finalized_block, environment_);

    GrandpaConfig config{.voters = std::move(voters),
                         .round_number = round_state.round_number,
                         .duration = round_time_factor_,
                         .id = keypair_
                                   ? boost::make_optional(keypair_->public_key)
                                   : boost::none};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, round_state.round_number, config.voters);

    auto new_round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        std::move(config),
        authority_manager_,
        environment_,
        std::move(vote_crypto_provider),
        std::make_shared<VoteTrackerImpl>(),  // Prevote tracker
        std::make_shared<VoteTrackerImpl>(),  // Precommit tracker
        std::move(vote_graph),
        clock_,
        io_context_,
        round_state);

    new_round->end();
    return new_round;
  }

  std::shared_ptr<VotingRound> GrandpaImpl::makeNextRound(
      const std::shared_ptr<VotingRound> &round) {
    BOOST_ASSERT(round->finalizedBlock().has_value());

    BlockInfo best_block = round->finalizedBlock().value();

    auto authorities_res = authority_manager_->authorities(best_block, true);
    if (authorities_res.has_error()) {
      SL_CRITICAL(logger_,
                  "Can't retrieve authorities for finalized block: {}",
                  authorities_res.error().message());
    }
    BOOST_ASSERT(authorities_res.has_value());

    auto &authorities = authorities_res.value();
    BOOST_ASSERT(not authorities->empty());

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                     authority.weight);
    }

    const auto new_round_number =
        round->voterSetId() == voters->id() ? (round->roundNumber() + 1) : 1;

    auto vote_graph = std::make_shared<VoteGraphImpl>(best_block, environment_);

    GrandpaConfig config{.voters = std::move(voters),
                         .round_number = new_round_number,
                         .duration = round_time_factor_,
                         .id = keypair_
                                   ? boost::make_optional(keypair_->public_key)
                                   : boost::none};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, new_round_number, config.voters);

    auto new_round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        std::move(config),
        authority_manager_,
        environment_,
        std::move(vote_crypto_provider),
        std::make_shared<VoteTrackerImpl>(),  // Prevote tracker
        std::make_shared<VoteTrackerImpl>(),  // Precommit tracker
        std::move(vote_graph),
        clock_,
        io_context_,
        round);
    return new_round;
  }

  std::shared_ptr<VotingRound> GrandpaImpl::selectRound(
      RoundNumber round_number) {
    std::shared_ptr<VotingRound> target_round;
    if (current_round_ && current_round_->roundNumber() == round_number) {
      target_round = current_round_;
    } else if (previous_round_
               && previous_round_->roundNumber() == round_number) {
      target_round = previous_round_;
    }
    return target_round;
  }

  outcome::result<MovableRoundState> GrandpaImpl::getLastCompletedRound()
      const {
    auto last_round_encoded_res = storage_->get(storage::kSetStateKey);

    // Saved data exists
    if (last_round_encoded_res.has_value()) {
      return scale::decode<MovableRoundState>(last_round_encoded_res.value());
    }

    // Fail at retrieve data
    if (last_round_encoded_res
        != outcome::failure(storage::DatabaseError::NOT_FOUND)) {
      return last_round_encoded_res.as_failure();
    }

    // No saved data - make from genesis
    auto genesis_hash_res = storage_->get(storage::kGenesisBlockHashLookupKey);
    if (not genesis_hash_res.has_value()) {
      logger_->critical("Can't retrieve genesis block hash: {}",
                        genesis_hash_res.error().message());
      return genesis_hash_res.as_failure();
    }

    primitives::BlockHash genesis_hash;
    std::copy(genesis_hash_res.value().begin(),
              genesis_hash_res.value().end(),
              genesis_hash.begin());

    return MovableRoundState{.round_number = 0,
                             .last_finalized_block = {0, genesis_hash},
                             .votes = {},
                             .finalized = {{0, genesis_hash}}};
  }

  void GrandpaImpl::executeNextRound() {
    previous_round_.swap(current_round_);
    previous_round_->end();
    current_round_ = makeNextRound(previous_round_);
    if (keypair_) {
      current_round_->play();
    }
  }

  void GrandpaImpl::onNeighborMessage(
      const libp2p::peer::PeerId &peer_id,
      const network::GrandpaNeighborMessage &msg) {
    SL_DEBUG(logger_,
             "NeighborMessage has received from {}: "
             "voter_set_id={} round={} last_finalized={}",
             peer_id.toBase58(),
             msg.voter_set_id,
             msg.round_number,
             msg.last_finalized);
    if (FullRound{msg} >= FullRound{current_round_}) {
      if (FullRound{prev_msg_} < FullRound{msg}) {
        auto res = environment_->onCatchUpRequested(
            peer_id, msg.voter_set_id, msg.round_number - 1);
        if (res) {
          prev_msg_ = FullRound{msg};
        }
      }
    }
  }

  void GrandpaImpl::onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                                     const network::CatchUpRequest &msg) {
    if (previous_round_ == nullptr) {
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "previous round is dummy yet",
          msg.round_number,
          peer_id.toBase58());
      return;
    }
    if (FullRound{msg} < FullRound{current_round_}) {
      // Catching up in to the past
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "catching up into the past",
          msg.round_number,
          peer_id.toBase58());
      return;
    }
    if (not previous_round_->completable()) {
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "round is not completable",
          msg.round_number,
          peer_id.toBase58());
      return;
    }
    if (not previous_round_->finalizable()) {
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "round is not finalizable",
          msg.round_number,
          peer_id.toBase58());
      return;
    }

    SL_DEBUG(logger_,
             "Catch-up request (since round #{}) received from {}",
             msg.round_number,
             peer_id.toBase58());
    previous_round_->doCatchUpResponse(peer_id);
  }

  void GrandpaImpl::onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                                      const network::CatchUpResponse &msg) {
    BOOST_ASSERT(current_round_ != nullptr);
    if (FullRound{msg} < FullRound{current_round_}) {
      // Catching up in to the past
      SL_DEBUG(
          logger_,
          "Catch-up response (till round #{}) received from {} was rejected: "
          "catching up into the past",
          msg.round_number,
          peer_id.toBase58());
      return;
    }

    SL_DEBUG(logger_,
             "Catch-up response (till round #{}) received from {}",
             msg.round_number,
             peer_id.toBase58());

    // TODO(xDimon): Probably will be cheaper update same round instead recreate
    // if (current_round_->roundNumber() == msg.round_number) {
    //  for (auto& prevote : msg.prevote_justification) {
    //    current_round_->onPrevote(prevote);
    //  }
    //  for (auto& precommit : msg.precommit_justification) {
    //    current_round_->onPrecommit(precommit);
    //  }
    //}

    MovableRoundState round_state{
        .round_number = msg.round_number,
        .last_finalized_block = current_round_->lastFinalizedBlock(),
        .votes = {},
        .finalized = msg.best_final_candidate};

    std::transform(msg.prevote_justification.begin(),
                   msg.prevote_justification.end(),
                   std::back_inserter(round_state.votes),
                   [](auto &item) { return item; });
    std::transform(msg.precommit_justification.begin(),
                   msg.precommit_justification.end(),
                   std::back_inserter(round_state.votes),
                   [](auto &item) { return item; });

    auto authorities_res =
        authority_manager_->authorities(round_state.finalized.value(), false);
    if (not authorities_res.has_value()) {
      SL_WARN(logger_,
              "Can't retrieve authorities for finalized block: {}",
              authorities_res.error().message());
      return;
    }
    auto &authorities = authorities_res.value();

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                     authority.weight);
    }

    auto round = makeInitialRound(round_state, std::move(voters));
    if (round == nullptr) {
      // Can't make round
      return;
    }

    // TODO(xDimon): Ensure if this ckecking is really needed
    // if (current_round_->bestPrevoteCandidate().number
    //     > round->bestFinalCandidate().number) {
    //   // GHOST-less Catch-up
    //   return;
    // }

    if (not round->completable()) {
      // Catch-up round is not completable
      return;
    }

    previous_round_.swap(current_round_);
    previous_round_->end();
    current_round_ = std::move(round);

    executeNextRound();
  }

  void GrandpaImpl::onVoteMessage(const libp2p::peer::PeerId &peer_id,
                                  const VoteMessage &msg) {
    SL_DEBUG(logger_,
             "{} has received from {}: "
             "voter_set_id={} round={} for block #{} hash={}",
             msg.vote.is<Prevote>()     ? "Prevote"
             : msg.vote.is<Precommit>() ? "Precommit"
                                        : "PrimaryPropose",
             peer_id.toBase58(),
             msg.counter,
             msg.round_number,
             msg.vote.getBlockNumber(),
             msg.vote.getBlockHash());

    std::shared_ptr<VotingRound> target_round = selectRound(msg.round_number);
    if (not target_round) {
      if (FullRound{current_round_} < FullRound{msg}) {
        if (FullRound{prev_msg_} < FullRound{msg}) {
          auto res = environment_->onCatchUpRequested(
              peer_id, msg.counter, msg.round_number);
          if (res) {
            prev_msg_ = FullRound{msg};
          }
        }
      }
      return;
    }

    // get block info
    auto blockInfo = visit_in_place(msg.vote.message, [](const auto &vote) {
      return BlockInfo(vote.number, vote.hash);
    });

    // get authorities
    // TODO (xdimon) reduce number of call to this runtime function
    const auto &weighted_authorities_res =
        grandpa_api_->authorities(primitives::BlockId(blockInfo.hash));
    if (!weighted_authorities_res.has_value()) {
      logger_->error("Can't get authorities: {}",
                     weighted_authorities_res.error().message());
      return;
    };
    auto &weighted_authorities = weighted_authorities_res.value();

    // find signer in authorities
    auto weighted_authority_it =
        std::find_if(weighted_authorities.begin(),
                     weighted_authorities.end(),
                     [&id = msg.vote.id](const auto &weighted_authority) {
                       return weighted_authority.id.id == id;
                     });

    if (weighted_authority_it == weighted_authorities.end()) {
      logger_->warn("Vote signed by unknown validator");
      return;
    };

    bool isPrevotesChanged = false;
    bool isPrecommitsChanged = false;
    visit_in_place(
        msg.vote.message,
        [&](const PrimaryPropose &) {
          target_round->onProposal(msg.vote,
                                   VotingRound::Propagation::REQUESTED);
        },
        [&](const Prevote &) {
          if (target_round->onPrevote(msg.vote,
                                      VotingRound::Propagation::REQUESTED)) {
            isPrevotesChanged = true;
          }
        },
        [&](const Precommit &) {
          if (target_round->onPrecommit(msg.vote,
                                        VotingRound::Propagation::REQUESTED)) {
            isPrecommitsChanged = true;
          }
        });
    target_round->update(isPrevotesChanged, isPrecommitsChanged);
  }

  void GrandpaImpl::onFinalize(const libp2p::peer::PeerId &peer_id,
                               const network::FullCommitMessage &fin) {
    SL_DEBUG(logger_,
             "Finalization of block #{} hash={} and voter set #{} has received "
             "from peer_id={}",
             fin.message.target_number,
             fin.message.target_hash.toHex(),
             fin.set_id,
             peer_id.toBase58());

    GrandpaJustification justification{
        .round_number = fin.round,
        .block_info =
            BlockInfo(fin.message.target_number, fin.message.target_hash)};
    for (size_t i = 0; i < fin.message.precommits.size(); ++i) {
      SignedPrecommit commit;
      commit.message = fin.message.precommits[i];
      commit.signature = fin.message.auth_data[i].first;
      commit.id = fin.message.auth_data[i].second;
      justification.items.emplace_back(std::move(commit));
    }

    auto res = applyJustification(justification.block_info, justification);
    if (not res.has_value()) {
      logger_->warn("Fin message is not applied: {}", res.error().message());
      return;
    }
  }

  outcome::result<void> GrandpaImpl::applyJustification(
      const BlockInfo &block_info, const GrandpaJustification &justification) {
    auto round = selectRound(justification.round_number);
    if (round == nullptr) {
      if (current_round_->bestPrevoteCandidate().number > block_info.number) {
        return VotingRoundError::JUSTIFICATION_FOR_BLOCK_IN_PAST;
      }

      MovableRoundState round_state{
          .round_number = justification.round_number,
          .last_finalized_block = current_round_->lastFinalizedBlock(),
          .votes = {},
          .finalized = block_info};

      auto authorities_res = authority_manager_->authorities(
          round_state.last_finalized_block, true);
      if (authorities_res.has_error()) {
        SL_WARN(logger_,
                "Can't retrieve authorities for applying justification "
                "of block #{} hash={}: {}",
                block_info.number,
                block_info.hash,
                authorities_res.error().message());
        return authorities_res.as_failure();
      }
      auto &authorities = authorities_res.value();

      auto voters = std::make_shared<VoterSet>(authorities->id);
      for (const auto &authority : *authorities) {
        voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                       authority.weight);
      }

      round = makeInitialRound(round_state, std::move(voters));
      BOOST_ASSERT(round);

      SL_DEBUG(logger_,
               "Rewind grandpa till round #{} by received justification",
               justification.round_number);
    }

    OUTCOME_TRY(round->applyJustification(block_info, justification));

    previous_round_.swap(current_round_);
    previous_round_->end();
    current_round_ = std::move(round);

    executeNextRound();

    return outcome::success();
  }

  void GrandpaImpl::onCompletedRound(
      outcome::result<MovableRoundState> round_state_res) {
    if (not round_state_res) {
      SL_DEBUG(logger_,
               "Grandpa round was not finalized: {}",
               round_state_res.error().message());
      return;
    }

    const auto &round_state = round_state_res.value();

    logger_->debug(
        "Save state of finalized round #{}: finalized={}, finalizing={}",
        round_state.round_number,
        round_state.last_finalized_block.number,
        round_state.finalized.value().number);

    SL_DEBUG(
        logger_, "Event OnCompleted for round #{}", round_state.round_number);

    if (auto put_res =
            storage_->put(storage::kSetStateKey,
                          common::Buffer(scale::encode(round_state).value()));
        not put_res) {
      logger_->error("New round state was not added to the storage");
      return;
    }

    BOOST_ASSERT(storage_->get(storage::kSetStateKey));
  }
}  // namespace kagome::consensus::grandpa
