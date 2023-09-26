/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/beefy/beefy.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "consensus/beefy/digest.hpp"
#include "consensus/beefy/sig.hpp"
#include "runtime/common/runtime_transaction_error.hpp"
#include "runtime/runtime_api/beefy.hpp"
#include "storage/spaced_storage.hpp"
#include "utils/block_number_key.hpp"

// TODO(turuslan): #1651, report equivocation

namespace kagome::network {
  Beefy::Beefy(application::AppStateManager &app_state_manager,
               std::shared_ptr<blockchain::BlockTree> block_tree,
               std::shared_ptr<runtime::BeefyApi> beefy_api,
               std::shared_ptr<crypto::EcdsaProvider> ecdsa,
               std::shared_ptr<storage::SpacedStorage> db,
               std::shared_ptr<primitives::events::ChainSubscriptionEngine>
                   chain_sub_engine)
      : block_tree_{std::move(block_tree)},
        beefy_api_{std::move(beefy_api)},
        ecdsa_{std::move(ecdsa)},
        db_{db->getSpace(storage::Space::kBeefyJustification)},
        log_{log::createLogger("Beefy")} {
    app_state_manager.atLaunch([=]() mutable {
      start(std::move(chain_sub_engine));
      return true;
    });
  }

  outcome::result<std::optional<consensus::beefy::BeefyJustification>>
  Beefy::getJustification(primitives::BlockNumber block) const {
    OUTCOME_TRY(raw, db_->tryGet(BlockNumberKey::encode(block)));
    if (raw) {
      OUTCOME_TRY(r, scale::decode<consensus::beefy::BeefyJustification>(*raw));
      return outcome::success(std::make_optional(std::move(r)));
    }
    return outcome::success(std::nullopt);
  }

  outcome::result<void> Beefy::onJustification(
      const primitives::BlockHash &block_hash, primitives::Justification raw) {
    if (not beefy_genesis_) {
      return outcome::success();
    }
    OUTCOME_TRY(justification_v1,
                scale::decode<consensus::beefy::BeefyJustification>(raw.data));
    auto &justification =
        boost::get<consensus::beefy::SignedCommitment>(justification_v1);
    OUTCOME_TRY(header, block_tree_->getBlockHeader(block_hash));
    if (justification.commitment.block_number != header.number) {
      return outcome::success();
    }
    return onJustification(std::move(justification));
  }

  void Beefy::onMessage(consensus::beefy::BeefyGossipMessage message) {
    if (not beefy_genesis_) {
      return;
    }
    if (auto justification_v1 =
            boost::get<consensus::beefy::BeefyJustification>(&message)) {
      auto &justification =
          boost::get<consensus::beefy::SignedCommitment>(*justification_v1);
      if (justification.commitment.block_number
          > block_tree_->bestLeaf().number) {
        return;
      }
      std::ignore = onJustification(std::move(justification));
    } else {
      auto &vote = boost::get<consensus::beefy::VoteMessage>(message);
      auto block_number = vote.commitment.block_number;
      if (block_number < *beefy_genesis_) {
        return;
      }
      if (block_number < beefy_finalized_) {
        return;
      }
      if (block_number >= next_digest_) {
        return;
      }
      auto next_session = sessions_.upper_bound(block_number);
      if (next_session == sessions_.begin()) {
        return;
      }
      auto session = std::prev(next_session)->second;
      if (vote.commitment.validator_set_id != session.validators.id) {
        return;
      }
      auto index = session.validators.find(vote.id);
      if (not index) {
        return;
      }
      if (not verify(*ecdsa_, vote)) {
        return;
      }
      // TODO(turuslan): #1651, collect votes
    }
  }

  void Beefy::start(std::shared_ptr<primitives::events::ChainSubscriptionEngine>
                        chain_sub_engine) {
    auto cursor = db_->cursor();
    std::ignore = cursor->seekLast();
    if (cursor->isValid()) {
      beefy_finalized_ = BlockNumberKey::decode(*cursor->key()).value();
    }
    SL_INFO(log_, "last finalized {}", beefy_finalized_);
    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        chain_sub_engine);
    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kFinalizedHeads);
    auto on_finalize = [weak = weak_from_this()](
                           subscription::SubscriptionSetId,
                           auto &&,
                           primitives::events::ChainEventType,
                           const primitives::events::ChainEventParams &) {
      if (auto self = weak.lock()) {
        std::ignore = self->update();
      }
    };
    chain_sub_->setCallback(std::move(on_finalize));
    std::ignore = update();
  }

  bool Beefy::hasJustification(primitives::BlockNumber block) const {
    auto r = db_->contains(BlockNumberKey::encode(block));
    return r and r.value();
  }

  outcome::result<Beefy::FindValidatorsResult> Beefy::findValidators(
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
          return runtime::RuntimeTransactionError::EXPORT_FUNCTION_NOT_FOUND;
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

  outcome::result<void> Beefy::onJustification(
      consensus::beefy::SignedCommitment justification) {
    auto block_number = justification.commitment.block_number;
    if (block_number < *beefy_genesis_) {
      return outcome::success();
    }
    pending_justifications_.emplace(block_number, std::move(justification));
    return update();
  }

  outcome::result<void> Beefy::apply(
      consensus::beefy::SignedCommitment justification) {
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
        return outcome::success();
      }
      session = std::prev(next_session);
      if (session == sessions_.end()) {
        return outcome::success();
      }
    }
    auto &first = found ? found->first : session->first;
    auto &validators = found ? found->second : session->second.validators;
    if (justification.commitment.validator_set_id != validators.id) {
      return outcome::success();
    }
    if (not verify(*ecdsa_, justification, validators)) {
      return outcome::success();
    }
    OUTCOME_TRY(db_->put(BlockNumberKey::encode(block_number),
                         scale::encode(consensus::beefy::BeefyJustification{
                                           std::move(justification),
                                       })
                             .value()));
    if (beefy_finalized_ != first) {
      OUTCOME_TRY(db_->remove(BlockNumberKey::encode(beefy_finalized_)));
    }
    if (block_number <= beefy_finalized_) {
      return outcome::success();
    }
    sessions_.erase(sessions_.begin(), session);
    if (found) {
      sessions_.emplace(found->first, Session{std::move(validators)});
    }
    SL_VERBOSE(log_, "finalized {}", block_number);
    beefy_finalized_ = block_number;
    next_digest_ = std::max(next_digest_, block_number + 1);
    return outcome::success();
  }

  outcome::result<void> Beefy::update() {
    auto grandpa_finalized = block_tree_->getLastFinalized();
    if (not beefy_genesis_) {
      BOOST_OUTCOME_TRY(beefy_genesis_,
                        beefy_api_->genesis(grandpa_finalized.hash));
      if (not beefy_genesis_) {
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
          apply(pending_justifications_.extract(pending_it++).mapped());
    }
    while (next_digest_ <= grandpa_finalized.number) {
      OUTCOME_TRY(
          found,
          findValidators(next_digest_,
                         sessions_.empty() ? *beefy_genesis_ : next_digest_));
      if (found) {
        sessions_.emplace(found->first, Session{std::move(found->second)});
      }
      ++next_digest_;
    }
    // TODO(turuslan): #1651, voting
    return outcome::success();
  }
}  // namespace kagome::network
