/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_impl.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/numeric.hpp>
#include "common/visitor.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  VotingRoundImpl::VotingRoundImpl(
      std::shared_ptr<VoterSet> voters,
      RoundNumber round_number,
      Duration duration,
      TimePoint start_time,
      MembershipCounter counter,
      RoundState last_round_state,
      crypto::ED25519Keypair keypair,
      std::shared_ptr<VoteTracker<Prevote>> prevotes,
      std::shared_ptr<VoteTracker<Precommit>> precommits,
      std::shared_ptr<Chain> chain,
      std::shared_ptr<VoteGraph> graph,
      std::shared_ptr<Gossiper> gossiper,
      std::shared_ptr<crypto::ED25519Provider> ed_provider,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      Timer timer,
      common::Logger logger)
      : voter_set_{std::move(voters)},
        round_number_{round_number},
        duration_{duration},
        start_time_{start_time},
        counter_{counter},
        last_round_state_{std::move(last_round_state)},
        keypair_{keypair},
        id_{keypair_.public_key},
        state_{State::START},
        prevotes_{std::move(prevotes)},
        precommits_{std::move(precommits)},
        chain_{std::move(chain)},
        graph_{std::move(graph)},
        gossiper_{std::move(gossiper)},
        ed_provider_{std::move(ed_provider)},
        clock_{std::move(clock)},
        block_tree_{std::move(block_tree)},
        timer_{std::move(timer)},
        logger_{std::move(logger)},
        prevote_equivocators_(voter_set_->size(), false),
        precommit_equivocators_(voter_set_->size(), false) {
    BOOST_ASSERT(voter_set_ != nullptr);
    BOOST_ASSERT(prevotes_ != nullptr);
    BOOST_ASSERT(precommits_ != nullptr);
    BOOST_ASSERT(chain_ != nullptr);
    BOOST_ASSERT(graph_ != nullptr);
    BOOST_ASSERT(gossiper_ != nullptr);
    BOOST_ASSERT(ed_provider_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);

    threshold_ = getThreshold(voter_set_);
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
    auto faulty = (voters->totalWeight() - 1) / 3;
    return voters->totalWeight() - faulty;
  }

  void VotingRoundImpl::onFin(const Fin &f) {
    // validate message
    switch (state_) {
      case State::PROPOSED:
      case State::START:
      case State::PREVOTED:
        break;
      case State::PRECOMMITTED: {
        tryFinalize();
      }
    }
  }

  /// This voter also implements the commit protocol.
  /// The commit protocol allows a node to broadcast a message that finalizes a
  /// given block and includes a set of precommits as proof.
  ///
  /// - When a round is completable and we precommitted we start a commit timer
  /// and start accepting commit messages;
  /// - When we receive a commit message if it targets a block higher than what
  /// we've finalized we validate it and import its precommits if valid;
  /// - When our commit timer triggers we check if we've received any commit
  /// message for a block equal to what we've finalized, if we haven't then we
  /// broadcast a commit.
  ///
  /// Additionally, we also listen to commit messages from rounds that aren't
  /// currently running, we validate the commit and dispatch a finalization
  /// notification (if any) to the environment.
  void VotingRoundImpl::tryFinalize() {
    if (not cur_round_state_.finalized or not completable_) {
      return;
    }
    auto l = block_tree_->getLastFinalized();

    auto e = cur_round_state_.estimate;

    if (not e) {
      return;
    }

    if (e->block_number > l.block_number) {
      const auto &opt_justification = finalizingPrecommits(e.value());
      if (not opt_justification) {
        logger_->warn("No justification for block  <{}, {}>",
                      e->block_number,
                      e->block_hash.toHex());
      }

      auto justification = opt_justification.value();

      primitives::Justification kagome_just{
          .data = common::Buffer{scale::encode(justification).value()}};

      auto finalized = block_tree_->finalize(e->block_hash, kagome_just);
      if (not finalized) {
        logger_->warn("Could not finalize message during round {}. Error: {}",
                      round_number_,
                      finalized.error().message());
      }

      // TODO (kamilsa): do not broadcast if fin message already received
      gossiper_->fin(Fin{.vote = e.value(),
                         .round_number = round_number_,
                         .justification = justification});
    }
  }

  void VotingRoundImpl::onPrimaryPropose(
      const SignedPrimaryPropose &primary_propose) {
    if (isPrimary(primary_propose.id)) {
      primaty_vote_ = primary_propose.message;
    }
  }

  void VotingRoundImpl::onPrevote(const SignedPrevote &prevote) {
    onSignedPrevote(prevote);
    updatePrevoteGhost();
    update();
    tryFinalize();
  }

  void VotingRoundImpl::onPrecommit(const SignedPrecommit &precommit) {
    onSignedPrecommit(precommit);
    update();
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
        // TODO (kamilsa): handle equivocated case
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          return;
        }
        prevote_equivocators_[index.value()] = true;
        break;
      }
    }

    // TODO: we use same timer in prevote and precommit. Might be trouble take a
    // look on that
    if (completable() and clock_->now() < timer_.expires_at()) {
      timer_.cancel();
    }
  }

  void VotingRoundImpl::onSignedPrecommit(const SignedPrecommit &vote) {
    auto weight = voter_set_->voterWeight(vote.id);
    if (not weight) {
      return;
    }
    switch (precommits_->push(vote, weight.value())) {
      case VoteTracker<Precommit>::PushResult::SUCCESS: {
        const auto &voters = voter_set_->voters();

        // prepare VoteWeight which contains index of who has voted and what
        // kind of vote it was
        VoteWeight v{voters.size()};
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          return;
        }

        v.precommits[index.value()] = voter_set_->voterWeight(vote.id).value();

        if (auto inserted = graph_->insert(vote.message, v); not inserted) {
          logger_->warn("Vote {} was not inserted with error: {}",
                        vote.message.block_hash.toHex(),
                        inserted.error().message());
        }
        break;
      }
      case VoteTracker<Precommit>::PushResult::DUPLICATED: {
        break;
      }
      case VoteTracker<Precommit>::PushResult::EQUIVOCATED: {
        // TODO (kamilsa): handle equivocated case
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          return;
        }
        precommit_equivocators_[index.value()] = true;
        break;
      }
    }

    // TODO: we use same timer in precommit and precommit. Might be trouble take
    // a look on that
    if (completable() and clock_->now() < timer_.expires_at()) {
      timer_.cancel();
    }
  }

  void VotingRoundImpl::updatePrevoteGhost() {
    if (prevotes_->totalWeight() >= threshold_) {
      const auto &ghost_block_info =
          cur_round_state_.prevote_ghost.map([](const Prevote &prevote) {
            return BlockInfo{prevote.block_number, prevote.block_hash};
          });
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
              .map([](const BlockInfo &ghost) {
                return Prevote{ghost.block_number, ghost.block_hash};
              });
    }
  }

  bool VotingRoundImpl::completable() const {
    return completable_;
  }

  void VotingRoundImpl::primaryPropose(const RoundState &last_round_state) {
    switch (state_) {
      case State::START: {
        auto maybe_estimate = last_round_state.estimate;

        if (maybe_estimate and isPrimary()) {
          auto maybe_finalized = last_round_state.finalized;

          // we should send primary if last round's state contains finalized
          // block and last round estimate is better than finalized block
          bool should_send_primary =
              maybe_finalized
                  .map([&maybe_estimate](const BlockInfo &finalized) {
                    return maybe_estimate->block_number
                           > finalized.block_number;
                  })
                  .value_or(false);

          if (should_send_primary) {
            logger_->debug("Sending primary block hint for round {}",
                           round_number_);
            primaty_vote_ = maybe_estimate.map([](const auto &estimate) {
              return PrimaryPropose{estimate.block_number, estimate.block_hash};
            });

            gossipPrimaryPropose(signPrimaryPropose(primaty_vote_.value()));

            state_ = State::PROPOSED;
          }
        } else {
          logger_->debug(
              "Last round estimate does not exist, not sending primary block "
              "hint during round {}",
              round_number_);
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
            gossipPrevote(prevote.value());
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
    timer_.expires_at(start_time_ + duration_ * 2);
    timer_.async_wait(handle_prevote);
  };

  void VotingRoundImpl::precommit(const RoundState &last_round_state) {
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
          if (not last_round_state.estimate) {
            logger_->warn("Rounds only started when prior round completable");
            return;
          }
          auto last_round_estimate = last_round_state.estimate.value();

          // We should precommit if current state contains prevote and it is
          // either equal to the last round estimate of is descendant of it
          bool should_precommit =
              cur_round_state_.prevote_ghost
                  .map([&](const Prevote &p_g) {
                    return p_g.block_hash == last_round_estimate.block_hash
                           or chain_->isEqualOrDescendOf(
                               last_round_estimate.block_hash, p_g.block_hash);
                  })
                  .value_or(false);

          if (should_precommit) {
            logger_->debug("Casting precommit for round {}", round_number_);

            auto precommit = constructPrecommit(last_round_state);

            if (precommit) {
              gossipPrecommit(precommit.value());
            }
            state_ = State::PRECOMMITTED;
          }
          break;
        }
        case State::START:
        case State::PROPOSED:
        case State::PRECOMMITTED:
          break;
      }
    };
    timer_.expires_at(start_time_ + duration_ * 4);
    timer_.async_wait(handle_precommit);
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
        primaty_vote_
            .map([](const PrimaryPropose &primary_propose) {
              return BlockInfo(primary_propose.block_number,
                               primary_propose.block_hash);
            })
            .map([&](const BlockInfo &primary) {
              // if we have primary_vote then:
              // if last prevote is the same as primary vote, then return it
              // else if primary vote is better than last prevote return
              auto last_prevote_g = last_round_state.prevote_ghost.value();

              if (std::tie(primary.block_number, primary.block_hash)
                  == std::tie(last_prevote_g.block_number,
                              last_prevote_g.block_hash)) {
                return primary;
              }
              if (primary.block_number >= last_prevote_g.block_number) {
                return last_round_estimate;
              }

              // from this point onwards, the number of the primary-broadcasted
              // block is less than the last prevote-GHOST's number.
              // if the primary block is in the ancestry of p-G we vote for the
              // best chain containing it.
              if (auto ancestry =
                      chain_->getAncestry(last_round_estimate.block_hash,
                                          last_prevote_g.block_hash);
                  ancestry) {
                auto to_sub = primary.block_number + 1;

                auto offset = 0;
                if (last_prevote_g.block_number >= to_sub) {
                  offset = last_prevote_g.block_number - to_sub;
                }

                if (ancestry.value().at(offset) == primary.block_hash) {
                  return primary;
                }
                return last_round_estimate;
              }
              return last_round_estimate;
            })
            .value_or_eval(
                [last_round_estimate] { return last_round_estimate; });

    auto rbest_chain =
        chain_->bestChainContaining(find_descendent_of.block_hash);

    if (not rbest_chain) {
      logger_->error(
          "Could not cast prevote: previousle known block {} has disappeared",
          find_descendent_of.block_hash.toHex());
      return outcome::failure(boost::system::error_code());
    }
    const auto &best_chain = rbest_chain.value();

    return signPrevote(Prevote{best_chain.block_number, best_chain.block_hash});
  }

  outcome::result<SignedPrecommit> Vot1ingRoundImpl::constructPrecommit(
      const RoundState &last_round_state) const {
    const auto &base = graph_->getBase();
    const auto &target = cur_round_state_.prevote_ghost.value_or(
        Prevote{base.block_number, base.block_hash});

    return signPrecommit(Precommit{
        target.block_number,
        target.block_hash,
    });
  }

  void VotingRoundImpl::update() {
    if (prevotes_->totalWeight() < threshold_) {
      return;
    }

    if (not cur_round_state_.prevote_ghost) {
      return;
    }

    auto prevote_ghost = cur_round_state_.prevote_ghost.value();

    // anything new finalized? finalized blocks are those which have both
    // 2/3+ prevote and precommit weight.
    auto current_precommits = precommits_->totalWeight();

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
    using namespace boost::adaptors;
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
          voter_set_->totalWeight() - precommits_->totalWeight();

      // all the votes already applied on this block,
      // assuming all remaining actors commit to this block,
      // and that we get further equivocations
      auto full_possible_weight =
          precommited_for + remaining_commit_votes + possible_equivocations;
      return full_possible_weight >= threshold_;
    };

    // until we have threshold precommits, any new block could get supermajority
    // precommits because there are at least f + 1 precommits remaining and then
    // f equivocations.
    //
    // once it's at least that level, we only need to consider blocks
    // already referenced in the graph, because no new leaf nodes
    // could ever have enough precommits.
    //
    // the round-estimate is the highest block in the chain with head
    // `prevote_ghost` that could have supermajority-commits.
    if (precommits_->totalWeight() >= threshold_) {
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

  boost::optional<Justification> VotingRoundImpl::finalizingPrecommits(
      const BlockInfo &estimate) const {
    const auto &precommits = precommits_->getMessages();
    Justification justification = std::accumulate(
        precommits.begin(),
        precommits.end(),
        Justification{},
        [this](Justification &j, const auto &precommit_variant) {
          visit_in_place(
              precommit_variant,
              [&j, this](const SignedPrecommit &voting_message) {
                if (chain_->isEqualOrDescendOf(
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

  void VotingRoundImpl::gossipPrimaryPropose(
      const SignedPrimaryPropose &primary_propose) {
    VoteMessage message{.vote = primary_propose,
                        .round_number = round_number_,
                        .counter = counter_,
                        .id = id_};
    gossiper_->vote(message);
  }

  void VotingRoundImpl::gossipPrevote(const SignedPrevote &prevote) {
    VoteMessage message{.vote = prevote,
                        .round_number = round_number_,
                        .counter = counter_,
                        .id = id_};
    gossiper_->vote(message);
  }

  void VotingRoundImpl::gossipPrecommit(const SignedPrecommit &precommit) {
    VoteMessage message{.vote = precommit,
                        .round_number = round_number_,
                        .counter = counter_,
                        .id = id_};
    gossiper_->vote(message);
  }

  template <typename VoteType>
  crypto::ED25519Signature VotingRoundImpl::voteSignature(
      uint8_t stage, const VoteType &vote_type) const {
    auto payload =
        scale::encode(stage, vote_type, round_number_, counter_).value();
    return ed_provider_->sign(keypair_, payload).value();
  }

  template crypto::ED25519Signature
  VotingRoundImpl::voteSignature<PrimaryPropose>(uint8_t,
                                                 const PrimaryPropose &) const;
  template crypto::ED25519Signature VotingRoundImpl::voteSignature<Prevote>(
      uint8_t, const Prevote &) const;
  template crypto::ED25519Signature VotingRoundImpl::voteSignature<Precommit>(
      uint8_t, const Precommit &) const;

  SignedPrimaryPropose VotingRoundImpl::signPrimaryPropose(
      const PrimaryPropose &primary_propose) const {
    const static uint8_t primary_propose_stage = 2;

    return SignedPrimaryPropose{
        .message = primary_propose,
        .id = id_,
        .signature = voteSignature(primary_propose_stage, primary_propose)};
  }

  SignedPrevote VotingRoundImpl::signPrevote(const Prevote &prevote) const {
    const static uint8_t prevote_stage = 0;

    return SignedPrevote{.message = prevote,
                         .id = id_,
                         .signature = voteSignature(prevote_stage, prevote)};
  }

  SignedPrecommit VotingRoundImpl::signPrecommit(
      const Precommit &precommit) const {
    const static uint8_t precommit_stage = 1;

    return SignedPrecommit{
        .message = precommit,
        .id = id_,
        .signature = voteSignature(precommit_stage, precommit)};
  }

}  // namespace kagome::consensus::grandpa
