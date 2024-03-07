/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/beefy/impl/beefy_impl.hpp"

#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>

#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "common/main_thread_pool.hpp"
#include "consensus/beefy/digest.hpp"
#include "consensus/beefy/impl/beefy_thread_pool.hpp"
#include "consensus/beefy/sig.hpp"
#include "consensus/timeline/timeline.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "metrics/histogram_timer.hpp"
#include "network/impl/protocols/beefy_protocol_impl.hpp"
#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/runtime_api/beefy.hpp"
#include "storage/spaced_storage.hpp"
#include "utils/block_number_key.hpp"

// TODO(turuslan): #1651, report equivocation
// TODO(turuslan): #1651, fetch justifications

namespace kagome::network {
  constexpr std::chrono::minutes kRebroadcastAfter{1};

  metrics::GaugeHelper metric_validator_set_id{
      "kagome_beefy_validator_set_id",
      "Current BEEFY active validator set id.",
  };
  metrics::GaugeHelper metric_finalized{
      "kagome_beefy_best_block",
      "Best block finalized by BEEFY",
  };

  BeefyImpl::BeefyImpl(
      application::AppStateManager &app_state_manager,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::BeefyApi> beefy_api,
      std::shared_ptr<crypto::EcdsaProvider> ecdsa,
      std::shared_ptr<storage::SpacedStorage> db,
      std::shared_ptr<common::MainPoolHandler> main_thread_handler,
      std::shared_ptr<BeefyThreadPool> beefy_thread_pool,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      LazySPtr<consensus::Timeline> timeline,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      LazySPtr<BeefyProtocol> beefy_protocol,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine)
      : block_tree_{std::move(block_tree)},
        beefy_api_{std::move(beefy_api)},
        ecdsa_{std::move(ecdsa)},
        db_{db->getSpace(storage::Space::kBeefyJustification)},
        main_pool_handler_(std::move(main_thread_handler)),
        beefy_pool_handler_{[&] {
          BOOST_ASSERT(beefy_thread_pool != nullptr);
          return beefy_thread_pool->handler();
        }()},
        scheduler_{std::move(scheduler)},
        timeline_{std::move(timeline)},
        session_keys_{std::move(session_keys)},
        beefy_protocol_{std::move(beefy_protocol)},
        min_delta_{chain_spec.beefyMinDelta()},
        chain_sub_{std::move(chain_sub_engine)},
        log_{log::createLogger("Beefy")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(beefy_api_ != nullptr);
    BOOST_ASSERT(ecdsa_ != nullptr);
    BOOST_ASSERT(db_ != nullptr);
    BOOST_ASSERT(main_pool_handler_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(session_keys_ != nullptr);

    app_state_manager.takeControl(*this);
  }

  primitives::BlockNumber BeefyImpl::finalized() const {
    return beefy_finalized_;
  }

  outcome::result<std::optional<consensus::beefy::BeefyJustification>>
  BeefyImpl::getJustification(primitives::BlockNumber block) const {
    OUTCOME_TRY(raw, db_->tryGet(BlockNumberKey::encode(block)));
    if (raw) {
      OUTCOME_TRY(r, scale::decode<consensus::beefy::BeefyJustification>(*raw));
      return outcome::success(std::make_optional(std::move(r)));
    }
    return outcome::success(std::nullopt);
  }

  void BeefyImpl::onJustification(const primitives::BlockHash &block_hash,
                                  primitives::Justification raw) {
    beefy_pool_handler_->execute(
        [weak{weak_from_this()}, block_hash, raw = std::move(raw)] {
          if (auto self = weak.lock()) {
            std::ignore =
                self->onJustificationOutcome(block_hash, std::move(raw));
          }
        });
  }

  outcome::result<void> BeefyImpl::onJustificationOutcome(
      const primitives::BlockHash &block_hash, primitives::Justification raw) {
    if (not beefy_genesis_) {
      return outcome::success();
    }
    OUTCOME_TRY(justification_v1,
                scale::decode<consensus::beefy::BeefyJustification>(raw.data));
    auto &justification =
        boost::get<consensus::beefy::SignedCommitment>(justification_v1);
    if (justification.commitment.block_number == beefy_finalized_) {
      return outcome::success();
    }
    OUTCOME_TRY(header, block_tree_->getBlockHeader(block_hash));
    if (justification.commitment.block_number != header.number) {
      return outcome::success();
    }
    return onJustification(std::move(justification));
  }

  void BeefyImpl::onMessage(consensus::beefy::BeefyGossipMessage message) {
    beefy_pool_handler_->execute(
        [weak{weak_from_this()}, message = std::move(message)] {
          if (auto self = weak.lock()) {
            self->onMessageStrand(std::move(message));
          }
        });
  }

  void BeefyImpl::onMessageStrand(
      consensus::beefy::BeefyGossipMessage message) {
    if (not beefy_genesis_) {
      return;
    }
    if (auto justification_v1 =
            boost::get<consensus::beefy::BeefyJustification>(&message)) {
      auto &justification =
          boost::get<consensus::beefy::SignedCommitment>(*justification_v1);
      if (justification.commitment.block_number == beefy_finalized_) {
        return;
      }
      if (justification.commitment.block_number
          > block_tree_->bestBlock().number) {
        return;
      }
      std::ignore = onJustification(std::move(justification));
    } else {
      onVote(boost::get<consensus::beefy::VoteMessage>(message), false);
    }
  }

  void BeefyImpl::onVote(consensus::beefy::VoteMessage vote, bool broadcast) {
    auto block_number = vote.commitment.block_number;
    if (block_number < *beefy_genesis_) {
      SL_VERBOSE(log_, "vote for block {} before genesis", block_number);
      return;
    }
    if (block_number <= beefy_finalized_) {
      return;
    }
    if (block_number >= next_digest_) {
      SL_VERBOSE(log_, "ignoring vote for unindexed block {}", block_number);
      return;
    }
    auto next_session = sessions_.upper_bound(block_number);
    if (next_session == sessions_.begin()) {
      return;
    }
    auto session_it = std::prev(next_session);
    auto &session = session_it->second;
    if (vote.commitment.validator_set_id != session.validators.id) {
      SL_VERBOSE(log_, "wrong validator set id for block {}", block_number);
      return;
    }
    auto index = session.validators.find(vote.id);
    if (not index) {
      SL_VERBOSE(log_, "unknown validator for block {}", block_number);
      return;
    }
    auto total = session.validators.validators.size();
    auto round = session.rounds.find(block_number);
    if (round != session.rounds.end() and round->second.signatures[*index]) {
      return;
    }
    if (not verify(*ecdsa_, vote)) {
      SL_VERBOSE(log_, "wrong vote for block {}", block_number);
      return;
    }
    auto commitment_ok = false;
    if (round != session.rounds.end()) {
      commitment_ok = vote.commitment == round->second.commitment;
    } else if (auto r = getCommitment(session.validators.id, block_number);
               not r or not r.value()) {
      return;
    } else {
      commitment_ok = vote.commitment == *r.value();
    }
    if (not commitment_ok) {
      SL_WARN(log_, "unexpected commitment for block {}", block_number);
      return;
    }
    if (round == session.rounds.end()) {
      round =
          session.rounds
              .emplace(block_number,
                       consensus::beefy::SignedCommitment{vote.commitment, {}})
              .first;
      round->second.signatures.resize(total);
    }
    round->second.signatures[*index] = vote.signature;
    size_t count = 0;
    for (auto &sig : round->second.signatures) {
      if (sig) {
        ++count;
      }
    }
    if (count >= consensus::beefy::threshold(total)) {
      std::ignore = apply(session.rounds.extract(round).mapped(), true);
    } else if (broadcast) {
      this->broadcast(std::move(vote));
    }
  }

  void BeefyImpl::prepare() {
    auto cursor = db_->cursor();
    std::ignore = cursor->seekLast();
    if (cursor->isValid()) {
      beefy_finalized_ = BlockNumberKey::decode(*cursor->key()).value();
      metric_finalized->set(beefy_finalized_);
    }
    SL_INFO(log_, "last finalized {}", beefy_finalized_);
    chain_sub_.onFinalize([weak{weak_from_this()}]() {
      if (auto self = weak.lock()) {
        self->beefy_pool_handler_->execute([weak] {
          if (auto self = weak.lock()) {
            std::ignore = self->update();
          }
        });
      }
    });
  }

  void BeefyImpl::start() {
    beefy_pool_handler_->start();
    beefy_pool_handler_->execute([weak{weak_from_this()}] {
      if (auto self = weak.lock()) {
        std::ignore = self->update();
      }
    });
    setTimer();
  }

  void BeefyImpl::stop() {
    beefy_pool_handler_->stop();
  }

  bool BeefyImpl::hasJustification(primitives::BlockNumber block) const {
    auto r = db_->contains(BlockNumberKey::encode(block));
    return r and r.value();
  }

  outcome::result<BeefyImpl::FindValidatorsResult> BeefyImpl::findValidators(
      primitives::BlockNumber max, primitives::BlockNumber min) const {
    OUTCOME_TRY(opt, block_tree_->getBlockHash(max));
    if (not opt) {
      return blockchain::BlockTreeError::HEADER_NOT_FOUND;
    }
    primitives::BlockInfo info{max, *opt};
    while (true) {
      if (info.number <= *beefy_genesis_) {
        // bug: beefy pallet doesn't produce digest with first validators
        OUTCOME_TRY(validators, beefy_api_->validatorSet(info.hash));
        if (not validators) {
          return runtime::RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND;
        }
        return std::make_pair(info.number, std::move(*validators));
      }
      OUTCOME_TRY(header, block_tree_->getBlockHeader(info.hash));
      if (auto validators = beefyValidatorsDigest(header)) {
        return std::make_pair(info.number, std::move(*validators));
      }
      if (info.number <= min) {
        return std::nullopt;
      }
      info = *header.parentInfo();
    }
  }

  outcome::result<void> BeefyImpl::onJustification(
      consensus::beefy::SignedCommitment justification) {
    auto block_number = justification.commitment.block_number;
    if (block_number < *beefy_genesis_) {
      return outcome::success();
    }
    pending_justifications_.emplace(block_number, std::move(justification));
    return update();
  }

  outcome::result<void> BeefyImpl::apply(
      consensus::beefy::SignedCommitment justification, bool broadcast) {
    auto block_number = justification.commitment.block_number;
    if (block_number == beefy_finalized_) {
      return outcome::success();
    }
    if (hasJustification(block_number)) {
      return outcome::success();
    }
    FindValidatorsResult found;
    if (block_number <= beefy_finalized_) {
      BOOST_OUTCOME_TRY(found, findValidators(block_number, block_number));
      if (not found) {
        return outcome::success();
      }
    } else if (block_number >= next_digest_) {
      BOOST_OUTCOME_TRY(found, findValidators(block_number, next_digest_));
    }
    auto session = sessions_.end();
    if (not found) {
      if (block_number <= beefy_finalized_) {
        return outcome::success();
      }
      auto next_session = sessions_.upper_bound(block_number);
      if (next_session == sessions_.begin()) {
        SL_TRACE(log_, "no session for block {}", block_number);
        return outcome::success();
      }
      session = std::prev(next_session);
    }
    auto &first = found ? found->first : session->first;
    auto &validators = found ? found->second : session->second.validators;
    if (justification.commitment.validator_set_id != validators.id) {
      SL_VERBOSE(log_, "wrong validator set id for block {}", block_number);
      return outcome::success();
    }
    if (not verify(*ecdsa_, justification, validators)) {
      SL_VERBOSE(log_, "wrong justification for block {}", block_number);
      return outcome::success();
    }
    consensus::beefy::BeefyJustification justification_v1{
        std::move(justification)};
    OUTCOME_TRY(db_->put(BlockNumberKey::encode(block_number),
                         scale::encode(justification_v1).value()));
    if (beefy_finalized_ > *beefy_genesis_
        and sessions_.count(beefy_finalized_) == 0) {
      OUTCOME_TRY(last_hash, block_tree_->getBlockHash(beefy_finalized_));
      if (last_hash) {
        if (auto r = block_tree_->getBlockHeader(*last_hash)) {
          if (not beefyValidatorsDigest(r.value())) {
            OUTCOME_TRY(db_->remove(BlockNumberKey::encode(beefy_finalized_)));
          }
        }
      }
    }
    if (block_number <= beefy_finalized_) {
      return outcome::success();
    }
    sessions_.erase(sessions_.begin(), session);
    if (session != sessions_.end()) {
      session->second.rounds.erase(
          session->second.rounds.begin(),
          session->second.rounds.upper_bound(block_number));
    }
    if (found) {
      sessions_.emplace(first, Session{std::move(validators), {}});
      metricValidatorSetId();
    }
    SL_INFO(log_, "finalized {}", block_number);
    beefy_finalized_ = block_number;
    metric_finalized->set(beefy_finalized_);
    next_digest_ = std::max(next_digest_, block_number + 1);
    if (broadcast) {
      this->broadcast(std::move(justification_v1));
    }
    beefy_pool_handler_->execute([weak{weak_from_this()}] {
      if (auto self = weak.lock()) {
        std::ignore = self->update();
      }
    });
    return outcome::success();
  }

  outcome::result<void> BeefyImpl::update() {
    auto grandpa_finalized = block_tree_->getLastFinalized();
    if (not beefy_genesis_) {
      BOOST_OUTCOME_TRY(beefy_genesis_,
                        beefy_api_->genesis(grandpa_finalized.hash));
      if (not beefy_genesis_) {
        SL_TRACE(log_, "no beefy pallet yet");
        return outcome::success();
      }
      next_digest_ = std::max(beefy_finalized_, *beefy_genesis_);
    }
    if (grandpa_finalized.number < *beefy_genesis_) {
      return outcome::success();
    }
    for (auto pending_it = pending_justifications_.begin();
         pending_it != pending_justifications_.end()
         and pending_it->first <= grandpa_finalized.number;) {
      std::ignore =
          apply(pending_justifications_.extract(pending_it++).mapped(), false);
    }
    while (next_digest_ <= grandpa_finalized.number) {
      OUTCOME_TRY(
          found,
          findValidators(next_digest_,
                         sessions_.empty() ? *beefy_genesis_ : next_digest_));
      if (found) {
        sessions_.emplace(found->first, Session{std::move(found->second), {}});
        metricValidatorSetId();
      }
      ++next_digest_;
    }
    std::ignore = vote();
    return outcome::success();
  }

  outcome::result<void> BeefyImpl::vote() {
    if (not timeline_.get()->wasSynchronized()) {
      return outcome::success();
    }
    auto next_session = sessions_.upper_bound(beefy_finalized_ + 1);
    if (next_session == sessions_.begin()) {
      if (next_session == sessions_.end()) {
        SL_VERBOSE(log_, "can't vote: no sessions");
        return outcome::success();
      }
      ++next_session;
    }
    auto session = std::prev(next_session);
    auto grandpa_finalized = block_tree_->getLastFinalized().number;
    auto target = session->first;
    if (target <= beefy_finalized_) {
      if (next_session != sessions_.end()
          and grandpa_finalized >= next_session->first) {
        target = next_session->first;
        session = next_session;
      } else {
        auto diff = grandpa_finalized - beefy_finalized_ + 1;
        target = beefy_finalized_
               + std::max<primitives::BlockNumber>(
                     min_delta_, math::nextHighPowerOf2(diff / 2));
        if (next_session != sessions_.end() and target >= next_session->first) {
          target = next_session->first;
          session = next_session;
        }
      }
    }
    if (target > grandpa_finalized) {
      return outcome::success();
    }
    if (target <= last_voted_) {
      return outcome::success();
    }
    auto key =
        session_keys_->getBeefKeyPair(session->second.validators.validators);
    if (not key) {
      SL_TRACE(log_,
               "can't vote: not validator of set {}",
               session->second.validators.id);
      return outcome::success();
    }
    OUTCOME_TRY(commitment,
                getCommitment(session->second.validators.id, target));
    if (not commitment) {
      SL_VERBOSE(log_, "can't vote: no commitment {}", target);
      return outcome::success();
    }
    OUTCOME_TRY(sig,
                ecdsa_->signPrehashed(consensus::beefy::prehash(*commitment),
                                      key->first->secret_key));
    consensus::beefy::VoteMessage vote{
        std::move(*commitment), key->first->public_key, sig};
    onVote(vote, true);
    last_voted_ = target;
    last_vote_ = std::move(vote);
    return outcome::success();
  }

  outcome::result<std::optional<consensus::beefy::Commitment>>
  BeefyImpl::getCommitment(consensus::beefy::AuthoritySetId validator_set_id,
                           primitives::BlockNumber block_number) {
    OUTCOME_TRY(block_hash, block_tree_->getBlockHash(block_number));
    if (not block_hash) {
      SL_VERBOSE(log_, "getCommitment: no block {}", block_number);
      return std::nullopt;
    }
    OUTCOME_TRY(header, block_tree_->getBlockHeader(*block_hash));
    auto mmr = beefyMmrDigest(header);
    if (not mmr) {
      SL_VERBOSE(
          log_, "getCommitment: no mmr digest in block {}", block_number);
      return std::nullopt;
    }
    return consensus::beefy::Commitment{
        {{consensus::beefy::kMmr, common::Buffer{*mmr}}},
        block_number,
        validator_set_id,
    };
  }

  void BeefyImpl::metricValidatorSetId() {
    if (not sessions_.empty()) {
      metric_validator_set_id->set(
          std::prev(sessions_.end())->second.validators.id);
    }
  }

  void BeefyImpl::broadcast(consensus::beefy::BeefyGossipMessage message) {
    REINVOKE(*main_pool_handler_, broadcast, std::move(message));
    beefy_protocol_.get()->broadcast(
        std::make_shared<consensus::beefy::BeefyGossipMessage>(
            std::move(message)));
    setTimer();
  }

  void BeefyImpl::setTimer() {
    REINVOKE(*main_pool_handler_, setTimer);
    auto f = [weak{weak_from_this()}] {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      auto f = [weak] {
        auto self = weak.lock();
        if (not self) {
          return;
        }
        if (not self->last_vote_) {
          return;
        }
        self->broadcast(*self->last_vote_);
      };
      self->beefy_pool_handler_->execute(std::move(f));
    };
    timer_ = scheduler_->scheduleWithHandle(std::move(f), kRebroadcastAfter);
  }
}  // namespace kagome::network
