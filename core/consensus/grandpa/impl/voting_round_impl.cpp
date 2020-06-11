/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_impl.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/numeric.hpp>
#include "common/visitor.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "primitives/justification.hpp"

namespace kagome::consensus::grandpa {

  static auto convertToPrimaryPropose = [](const auto &vote) {
    return PrimaryPropose(vote.block_number, vote.block_hash);
  };

  static auto convertToPrevote = [](const auto &vote) {
    return Prevote(vote.block_number, vote.block_hash);
  };

  static auto convertToBlockInfo = [](const auto &vote) {
    return BlockInfo(vote.block_number, vote.block_hash);
  };

  VotingRoundImpl::VotingRoundImpl(
      const GrandpaConfig &config,
      std::shared_ptr<Environment> env,
      std::shared_ptr<VoteCryptoProvider> vote_crypto_provider,
      std::shared_ptr<VoteTracker<Prevote>> prevotes,
      std::shared_ptr<VoteTracker<Precommit>> precommits,
      std::shared_ptr<VoteGraph> graph,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context)
      : voter_set_{config.voters},
        round_number_{config.round_number},
        duration_{config.duration},
        id_{config.peer_id},
        env_{std::move(env)},
        vote_crypto_provider_{std::move(vote_crypto_provider)},
        state_{State::START},
        prevotes_{std::move(prevotes)},
        precommits_{std::move(precommits)},
        graph_{std::move(graph)},
        clock_{std::move(clock)},
        io_context_{std::move(io_context)},
        prevote_timer_{*io_context_},
        precommit_timer_{*io_context_},
        logger_{common::createLogger("Grandpa")},
        prevote_equivocators_(voter_set_->size(), false),
        precommit_equivocators_(voter_set_->size(), false) {
    BOOST_ASSERT(voter_set_ != nullptr);
    BOOST_ASSERT(vote_crypto_provider_ != nullptr);
    BOOST_ASSERT(prevotes_ != nullptr);
    BOOST_ASSERT(precommits_ != nullptr);
    BOOST_ASSERT(env_ != nullptr);
    BOOST_ASSERT(graph_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);

    threshold_ = getThreshold(voter_set_);
    start_time_ = clock_->now();
  }

  bool VotingRoundImpl::isPrimary(const Id &id) const {
    auto index = round_number_ % voter_set_->size();
    return voter_set_->voters().at(index) == id;
  }

  bool VotingRoundImpl::isPrimary() const {
    return isPrimary(id_);
  }

  size_t VotingRoundImpl::getThreshold(
      const std::shared_ptr<VoterSet> &voters) {
    // calculate supermajority
    auto faulty = (voters->totalWeight() - 1) / 3;
    return voters->totalWeight() - faulty;
  }

  void VotingRoundImpl::onFinalize(const Fin &f) {
    logger_->debug("Received fin message for vote: {}",
                   f.vote.block_hash.toHex());
    // validate message
    if (validate(f.vote, f.justification)) {
      // finalize to state
      auto finalized = env_->finalize(f.vote.block_hash, f.justification);
      if (not finalized) {
        logger_->error(
            "Could not finalize block {} from round {} with error: {}",
            f.vote.block_hash.toHex(),
            f.round_number,
            finalized.error().message());
        return;
      }
      env_->onCompleted(CompletedRound{.round_number = round_number_,
                                       .state = cur_round_state_});
    } else {
      logger_->error("Validation of vote {} failed", f.vote.block_hash.toHex());
      env_->onCompleted(VotingRoundError::FIN_VALIDATION_FAILED);
    }
  }

  bool VotingRoundImpl::validate(
      const BlockInfo &vote, const GrandpaJustification &justification) const {
    size_t total_weight = 0;
    for (const auto &signed_precommit : justification.items) {
      // verify signatures
      if (not vote_crypto_provider_->verifyPrecommit(signed_precommit)) {
        logger_->error(
            "Received invalid signed precommit during the round {} from the "
            "peer {}",
            round_number_,
            signed_precommit.id.toHex());
        return false;
      }

      // check that every signed precommit corresponds to the vote (i.e.
      // signed_precommits are descendants of the vote). If so add weight of
      // that voter to the total weight
      if (env_->getAncestry(vote.block_hash,
                            signed_precommit.message.block_hash)) {
        total_weight +=
            voter_set_->voterWeight(signed_precommit.id)
                .value_or(0);  // add zero if such voter does not exist
      }
    }

    return total_weight >= threshold_;
  }

  bool VotingRoundImpl::tryFinalize() {
    if (not completable()) {
      return false;
    }
    if (not last_round_state_) {
      logger_->error("Last round state is empty during finalization");
      return false;
    }
    // check if new state differs with the old one and broadcast new state
    if (auto notify_res = notify(*last_round_state_); not notify_res) {
      logger_->debug("Did not notify. Reason: {}",
                     notify_res.error().message());
      // Round is completable but we cannot notify others. Finish the round
      env_->onCompleted(notify_res.error());
      return false;
    }
    return true;
  }

  outcome::result<void> VotingRoundImpl::notify(
      const RoundState &last_round_state) {
    if (completable_) {
      auto finalized = cur_round_state_.finalized.value();
      const auto &opt_justification = finalizingPrecommits(finalized);
      if (not opt_justification) {
        logger_->warn("No justification for block  <{}, {}>",
                      finalized.block_number,
                      finalized.block_hash.toHex());
      }

      auto justification = opt_justification.value();

      OUTCOME_TRY(env_->onCommitted(round_number_, finalized, justification));
      return outcome::success();
    }
    return VotingRoundError::NEW_STATE_EQUAL_TO_OLD;
  }

  RoundNumber VotingRoundImpl::roundNumber() const {
    return round_number_;
  }

  void VotingRoundImpl::onPrimaryPropose(
      const SignedPrimaryPropose &primary_propose) {
    if (isPrimary(primary_propose.id)) {
      primary_vote_ = primary_propose.message;
    }
  }

  void VotingRoundImpl::onPrevote(const SignedPrevote &prevote) {
    onSignedPrevote(prevote);
    updatePrevoteGhost();
    update();

    // stop prevote timer if round is completable
    if (completable() and clock_->now() < prevote_timer_.expires_at()) {
      prevote_timer_.cancel();
    }
    tryFinalize();
  }

  void VotingRoundImpl::onPrecommit(const SignedPrecommit &precommit) {
    if (not onSignedPrecommit(precommit)) {
      env_->onCompleted(VotingRoundError::LAST_ESTIMATE_BETTER_THAN_PREVOTE);
      return;
    }
    update();

    // stop precommit timer if round is completable
    if (completable() and clock_->now() < precommit_timer_.expires_at()) {
      precommit_timer_.cancel();
    }
    tryFinalize();
  }

  void VotingRoundImpl::onSignedPrevote(const SignedPrevote &vote) {
    auto weight = voter_set_->voterWeight(vote.id);
    if (not weight) {
      return;
    }
    switch (prevotes_->push(vote, weight.value())) {
      case VoteTracker<Prevote>::PushResult::SUCCESS: {
        const auto &voters = voter_set_->voters();

        // prepare VoteWeight which contains index of who has voted and what
        // kind of vote it was
        VoteWeight v{voters.size()};
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          logger_->warn("Voter {} is not known: {}", vote.id.toHex());
          return;
        }

        v.prevotes[index.value()] = voter_set_->voterWeight(vote.id).value();

        if (auto inserted = graph_->insert(vote.message, v); not inserted) {
          logger_->warn("Vote {} was not inserted with error: {}",
                        vote.message.block_hash.toHex(),
                        inserted.error().message());
        }
        break;
      }
      case VoteTracker<Prevote>::PushResult::DUPLICATED: {
        break;
      }
      case VoteTracker<Prevote>::PushResult::EQUIVOCATED: {
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          logger_->warn("Voter {} is not known: {}", vote.id.toHex());
          return;
        }
        prevote_equivocators_[index.value()] = true;
        break;
      }
    }
  }

  bool VotingRoundImpl::onSignedPrecommit(const SignedPrecommit &vote) {
    auto weight = voter_set_->voterWeight(vote.id);
    if (not weight) {
      return false;
    }
    switch (precommits_->push(vote, weight.value())) {
      case VoteTracker<Precommit>::PushResult::SUCCESS: {
        const auto &voters = voter_set_->voters();

        // prepare VoteWeight which contains index of who has voted and what
        // kind of vote it was
        VoteWeight v{voters.size()};
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          logger_->warn("Voter {} is not known: {}", vote.id.toHex());
          return false;
        }

        v.precommits[index.value()] = voter_set_->voterWeight(vote.id).value();

        if (auto inserted = graph_->insert(vote.message, v); not inserted) {
          logger_->warn("Vote {} was not inserted with error: {}",
                        vote.message.block_hash.toHex(),
                        inserted.error().message());
          return false;
        }
        break;
      }
      case VoteTracker<Precommit>::PushResult::DUPLICATED: {
        return false;
      }
      case VoteTracker<Precommit>::PushResult::EQUIVOCATED: {
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          logger_->warn("Voter {} is not known: {}", vote.id.toHex());
          return false;
        }
        precommit_equivocators_[index.value()] = true;
        break;
      }
    }
    return true;
  }

  void VotingRoundImpl::updatePrevoteGhost() {
    if (prevotes_->getTotalWeight() >= threshold_) {
      const auto &ghost_block_info =
          cur_round_state_.prevote_ghost.map(convertToBlockInfo);
      cur_round_state_.prevote_ghost =
          graph_
              ->findGhost(ghost_block_info,
                          [this](const VoteWeight &vote_weight) {
                            return vote_weight
                                       .totalWeight(prevote_equivocators_,
                                                    precommit_equivocators_,
                                                    voter_set_)
                                       .prevote
                                   >= threshold_;
                          })
              // convert block info to prevote
              .map(convertToPrevote);
    }
  }

  bool VotingRoundImpl::completable() const {
    return completable_;
  }

  void VotingRoundImpl::primaryPropose(const RoundState &last_round_state) {
    last_round_state_ = last_round_state;
    switch (state_) {
      case State::START: {
        if (not isPrimary()) {
          break;
        }

        const auto &maybe_estimate = last_round_state.estimate;

        if (not maybe_estimate) {
          logger_->debug(
              "Last round estimate does not exist, not sending primary block "
              "hint during round {}",
              round_number_);
          break;
        }

        const auto &maybe_finalized = last_round_state.finalized;

        // we should send primary if last round's state contains finalized
        // block and last round estimate is better than finalized block
        bool should_send_primary =
            maybe_finalized
                .map([&maybe_estimate](const BlockInfo &finalized) {
                  return maybe_estimate->block_number > finalized.block_number;
                })
                .value_or(false);

        if (should_send_primary) {
          logger_->debug("Sending primary block hint for round {}",
                         round_number_);
          primary_vote_ = maybe_estimate.map(convertToPrimaryPropose);

          auto proposed = env_->onProposed(
              round_number_,
              voter_set_->setId(),
              vote_crypto_provider_->signPrimaryPropose(primary_vote_.value()));
          if (not proposed) {
            logger_->error("Primary propose was not sent: {}",
                           proposed.error().message());
            break;
          }
          state_ = State::PROPOSED;
        }
        break;
      }
      case State::PROPOSED:
      case State::PREVOTED:
      case State::PRECOMMITTED:
        break;
    }
  }

  void VotingRoundImpl::prevote(const RoundState &last_round_state) {
    last_round_state_ = last_round_state;

    auto handle_prevote = [this, last_round_state](auto &&ec) {
      // Return if error is not caused by timer cancellation
      if (ec and ec != boost::asio::error::operation_aborted) {
        logger_->error("Error happened during prevote timer: {}", ec.message());
        return;
      }
      switch (state_) {
        case State::START:
        case State::PROPOSED: {
          auto prevote = constructPrevote(last_round_state);
          if (prevote) {
            auto prevoted = env_->onPrevoted(
                round_number_, voter_set_->setId(), prevote.value());
            if (not prevoted) {
              logger_->error("Prevote was not sent: {}",
                             prevoted.error().message());
            }
          }
          state_ = State::PREVOTED;
          break;
        }
        case State::PREVOTED:
        case State::PRECOMMITTED: {
          break;
        }
      }
    };
    // wait as spec suggests
    prevote_timer_.expires_at(start_time_ + duration_ * 2);
    prevote_timer_.async_wait(handle_prevote);
  };

  void VotingRoundImpl::precommit(const RoundState &last_round_state) {
    last_round_state_ = last_round_state;

    auto handle_precommit = [this, last_round_state](
                                const boost::system::error_code &ec) {
      // Return if error is not caused by timer cancellation
      if (ec and ec != boost::asio::error::operation_aborted) {
        logger_->error("Error happened during precommit timer: {}",
                       ec.message());
        return;
      }

      switch (state_) {
        case State::PREVOTED: {
          state_ = State::PRECOMMITTED;
          if (not last_round_state.estimate) {
            logger_->warn("Rounds only started when prior round completable");
            return;
          }
          auto last_round_estimate = last_round_state.estimate.value();

          // We should precommit if current state contains prevote and it is
          // either equal to the last round estimate or is descendant of it
          bool should_precommit =
              cur_round_state_.prevote_ghost
                  .map([&](const Prevote &p_g) {
                    return p_g.block_hash == last_round_estimate.block_hash
                           or env_->isEqualOrDescendOf(
                               last_round_estimate.block_hash, p_g.block_hash);
                  })
                  .value_or(false);

          if (should_precommit) {
            logger_->debug("Casting precommit for round {}", round_number_);

            auto precommit = constructPrecommit(last_round_state);

            if (precommit) {
              auto precommitted = env_->onPrecommitted(
                  round_number_, voter_set_->setId(), precommit.value());
              if (not precommitted) {
                logger_->error("Precommit was not sent: {}",
                               precommitted.error().message());
                break;
              }
              state_ = State::PRECOMMITTED;
              break;
            }
            BOOST_ASSERT_MSG(false, "Not possible. Shouldn't get here");
          }
          env_->onCompleted(
              VotingRoundError::LAST_ESTIMATE_BETTER_THAN_PREVOTE);
          break;
        }
        case State::START:
        case State::PROPOSED:
        case State::PRECOMMITTED:
          break;
      }
    };
    // wait as spec suggests
    precommit_timer_.expires_at(start_time_ + duration_ * 4);
    precommit_timer_.async_wait(handle_precommit);
  }

  outcome::result<SignedPrevote> VotingRoundImpl::constructPrevote(
      const RoundState &last_round_state) const {
    if (not last_round_state.estimate) {
      logger_->warn("Rounds only started when prior round completable");
      return outcome::failure(boost::system::error_code());
    }
    auto last_round_estimate = last_round_state.estimate.value();

    // find the block that we should take descendent from
    auto find_descendent_of =
        primary_vote_.map(convertToBlockInfo)
            .map([&](const BlockInfo &primary) {
              // if we have primary_vote then:
              // if last prevote is the same as primary vote, then return it
              // else if primary vote is better than last prevote return last
              // round estimate
              auto last_prevote_g = last_round_state.prevote_ghost.value();

              if (std::tie(primary.block_number, primary.block_hash)
                  == std::tie(last_prevote_g.block_number,
                              last_prevote_g.block_hash)) {
                return primary;
              }
              if (primary.block_number >= last_prevote_g.block_number) {
                return last_round_estimate;
              }

              // from this point onwards, the number of the
              // primary-broadcasted block is less than the last
              // prevote-GHOST's number. if the primary block is in the
              // ancestry of p-G we vote for the best chain containing it.
              if (auto ancestry =
                      env_->getAncestry(last_round_estimate.block_hash,
                                        last_prevote_g.block_hash);
                  ancestry) {
                auto to_sub = primary.block_number + 1;

                primitives::BlockNumber offset = 0;
                if (last_prevote_g.block_number >= to_sub) {
                  offset = last_prevote_g.block_number - to_sub;
                }

                if (ancestry.value().at(offset) == primary.block_hash) {
                  return primary;
                }
              }
              return last_round_estimate;
            })
            .value_or_eval(
                [last_round_estimate] { return last_round_estimate; });

    auto rbest_chain = env_->bestChainContaining(find_descendent_of.block_hash);

    if (not rbest_chain) {
      logger_->error(
          "Could not cast prevote: previously known block {} has disappeared",
          find_descendent_of.block_hash.toHex());
      return outcome::failure(boost::system::error_code());
    }
    const auto &best_chain = rbest_chain.value();

    return vote_crypto_provider_->signPrevote(
        {best_chain.block_number, best_chain.block_hash});
  }

  outcome::result<SignedPrecommit> VotingRoundImpl::constructPrecommit(
      const RoundState &last_round_state) const {
    const auto &base = graph_->getBase();
    const auto &target = cur_round_state_.prevote_ghost.value_or(
        Prevote{base.block_number, base.block_hash});

    return vote_crypto_provider_->signPrecommit({
        target.block_number,
        target.block_hash,
    });
  }

  void VotingRoundImpl::update() {
    if (prevotes_->getTotalWeight() < threshold_) {
      return;
    }

    if (not cur_round_state_.prevote_ghost) {
      return;
    }

    auto prevote_ghost = cur_round_state_.prevote_ghost.value();

    // anything new finalized? finalized blocks are those which have both
    // 2/3+ prevote and precommit weight.
    auto current_precommits = precommits_->getTotalWeight();

    if (current_precommits >= threshold_) {
      cur_round_state_.finalized = graph_->findAncestor(
          BlockInfo{
              prevote_ghost.block_number,
              prevote_ghost.block_hash,
          },
          [this](const VoteWeight &vote_weight) {
            return vote_weight
                       .totalWeight(prevote_equivocators_,
                                    precommit_equivocators_,
                                    voter_set_)
                       .precommit
                   >= threshold_;
          });
    }

    // figuring out whether a block can still be committed for is
    // not straightforward because we have to account for all possible future
    // equivocations and thus cannot discount weight from validators who
    // have already voted.
    auto tolerated_equivocations = voter_set_->totalWeight() - threshold_;

    // find how many more equivocations we could still get.
    //
    // it is only important to consider the voters whose votes
    // we have already seen, because we are assuming any votes we
    // haven't seen will target this block.

    // get total weight of all equivocators
    using namespace boost::adaptors;  // NOLINT
    auto current_equivocations = boost::accumulate(
        voter_set_->voters() | indexed(0)
            | transformed([this](const auto &element) {
                return precommit_equivocators_[element.index()]
                           ? voter_set_->voterWeight(element.index()).value()
                           : 0UL;
              }),
        0UL);

    auto additional_equiv = tolerated_equivocations - current_equivocations;
    auto possible_to_precommit = [&](const VoteWeight &weight) {
      // total precommits for this block, including equivocations.
      auto precommited_for =
          weight
              .totalWeight(
                  prevote_equivocators_, precommit_equivocators_, voter_set_)
              .precommit;

      // equivocations we could still get are out of those who
      // have already voted, but not on this block.
      auto possible_equivocations = std::min<size_t>(
          current_precommits - precommited_for, additional_equiv);

      auto remaining_commit_votes =
          voter_set_->totalWeight() - precommits_->getTotalWeight();

      // all the votes already applied on this block,
      // assuming all remaining actors commit to this block,
      // and that we get further equivocations
      auto full_possible_weight =
          precommited_for + remaining_commit_votes + possible_equivocations;
      return full_possible_weight >= threshold_;
    };

    // until we have threshold precommits, any new block could get
    // supermajority precommits because there are at least f + 1 precommits
    // remaining and then f equivocations.
    //
    // once it's at least that level, we only need to consider blocks
    // already referenced in the graph, because no new leaf nodes
    // could ever have enough precommits.
    //
    // the round-estimate is the highest block in the chain with head
    // `prevote_ghost` that could have supermajority-commits.
    if (precommits_->getTotalWeight() >= threshold_) {
      cur_round_state_.estimate = graph_->findAncestor(
          BlockInfo{prevote_ghost.block_number, prevote_ghost.block_hash},
          possible_to_precommit);
    } else {
      cur_round_state_.estimate =
          BlockInfo{prevote_ghost.block_number, prevote_ghost.block_hash};
      return;
    }

    completable_ =
        cur_round_state_.estimate
            .map([&prevote_ghost, &possible_to_precommit, this](
                     const BlockInfo &block) -> bool {
              return block.block_hash != prevote_ghost.block_hash
                     || graph_->findGhost(block, possible_to_precommit)
                            .map([&prevote_ghost](const BlockInfo &block_info) {
                              return std::tie(block_info.block_hash,
                                              block_info.block_number)
                                     == std::tie(prevote_ghost.block_hash,
                                                 prevote_ghost.block_number);
                            })
                            .value_or(true);
            })
            .value_or(false);
  }

  boost::optional<GrandpaJustification> VotingRoundImpl::finalizingPrecommits(
      const BlockInfo &estimate) const {
    const auto &precommits = precommits_->getMessages();
    GrandpaJustification justification = std::accumulate(
        precommits.begin(),
        precommits.end(),
        GrandpaJustification{},
        [this](GrandpaJustification &j, const auto &precommit_variant) {
          visit_in_place(
              precommit_variant,
              [&j, this](const SignedPrecommit &voting_message) {
                if (env_->isEqualOrDescendOf(
                        cur_round_state_.finalized->block_hash,
                        voting_message.message.block_hash)) {
                  j.items.push_back(voting_message);
                }
              },
              [&](const VoteTracker<Precommit>::EquivocatoryVotingMessage
                      &equivocatory_voting_message) {
                j.items.push_back(equivocatory_voting_message.first);
                j.items.push_back(equivocatory_voting_message.second);
              });
          return j;
        });
    return justification;
  }

}  // namespace kagome::consensus::grandpa
