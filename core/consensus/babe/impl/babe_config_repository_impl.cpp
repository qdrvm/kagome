/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babe_config_repository_impl.hpp"

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "babe_digests_util.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "consensus/babe/babe_error.hpp"
#include "crypto/hasher.hpp"
#include "primitives/block_header.hpp"
#include "runtime/runtime_api/babe_api.hpp"
#include "scale/scale.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/trie_storage.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::babe,
                            BabeConfigRepositoryImpl::Error,
                            e) {
  using E = decltype(e);
  switch (e) {
    case E::NOT_FOUND:
      return "babe config not found";
    case E::PREVIOUS_NOT_FOUND:
      return "previous babe config not found";
  }
  return fmt::format("BabeConfigRepositoryImpl::Error({})", e);
}

namespace kagome::consensus::babe {
  /**
   * If there is more than `kMaxUnindexedBlocksNum` unindexed finalized blocks
   * and last finalized block has state, then babe won't index all of them, but
   * recover with runtime call and latest block with digest.
   */
  constexpr size_t kMaxUnindexedBlocksNum = 10000;

  inline static primitives::NextConfigDataV1 getConfig(
      const primitives::BabeConfiguration &state) {
    return {state.leadership_rate, state.allowed_slots};
  }

  BabeConfigRepositoryImpl::BabeConfigRepositoryImpl(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<storage::SpacedStorage> persistent_storage,
      const application::AppConfiguration &app_config,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<runtime::BabeApi> babe_api,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      const BabeClock &clock)
      : persistent_storage_(
          persistent_storage->getSpace(storage::Space::kDefault)),
        config_warp_sync_{app_config.syncMethod()
                          == application::AppConfiguration::SyncMethod::Warp},
        block_tree_(std::move(block_tree)),
        indexer_{
            std::make_shared<storage::MapPrefix>(
                storage::kBabeConfigRepositoryImplIndexerPrefix,
                persistent_storage_),
            block_tree_,
        },
        header_repo_(std::move(header_repo)),
        babe_api_(std::move(babe_api)),
        hasher_(std::move(hasher)),
        trie_storage_(std::move(trie_storage)),
        chain_sub_([&] {
          BOOST_ASSERT(chain_events_engine != nullptr);
          return std::make_shared<primitives::events::ChainEventSubscriber>(
              chain_events_engine);
        }()),
        clock_(clock),
        logger_(log::createLogger("BabeConfigRepo", "babe_config_repo")) {
    BOOST_ASSERT(persistent_storage_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(babe_api_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    if (auto r = indexer_.init(); not r) {
      logger_->error("Indexer::init error: {}", r.error());
    }

    app_state_manager.atPrepare([this] { return prepare(); });
  }

  bool BabeConfigRepositoryImpl::prepare() {
    auto finalized = block_tree_->getLastFinalized();
    auto finalized_header = block_tree_->getBlockHeader(finalized.hash).value();

    primitives::BlockNumber indexer_last_finalized;
    {
      std::unique_lock lock{indexer_mutex_};
      indexer_last_finalized = indexer_.last_finalized_indexed_.number;
    }

    if (finalized.number - indexer_last_finalized
            > kMaxUnindexedBlocksNum
        and trie_storage_->getEphemeralBatchAt(finalized_header.state_root)) {
      warp(finalized);
    }

    auto genesis_res = config({block_tree_->getGenesisBlockHash(), 0}, false);
    if (not genesis_res) {
      SL_ERROR(logger_, "get config at genesis error: {}", genesis_res.error());
      return false;
    }
    auto &genesis = genesis_res.value();
    slot_duration_ = genesis->slot_duration;
    epoch_length_ = genesis->epoch_length;

    auto best = block_tree_->bestLeaf();
    auto best_header = block_tree_->getBlockHeader(best.hash).value();
    if (auto res = config(best, true); not res and not config_warp_sync_) {
      SL_ERROR(logger_, "get config at best {} error: {}", best, res.error());
      if (not trie_storage_->getEphemeralBatchAt(best_header.state_root)) {
        SL_ERROR(logger_,
                 "warp sync was not completed, restart with \"--sync Warp\"");
      }
      return false;
    }

    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kFinalizedHeads);
    chain_sub_->setCallback(
        [wp = weak_from_this()](
            subscription::SubscriptionSetId,
            auto &&,
            primitives::events::ChainEventType type,
            const primitives::events::ChainEventParams &event) {
          if (type == primitives::events::ChainEventType::kFinalizedHeads) {
            if (auto self = wp.lock()) {
              std::unique_lock lock{self->indexer_mutex_};
              self->indexer_.finalize();
            }
          }
        });

    return true;
  }

  outcome::result<std::shared_ptr<const primitives::BabeConfiguration>>
  BabeConfigRepositoryImpl::config(const primitives::BlockInfo &parent_info,
                                   EpochNumber epoch_number) const {
    auto epoch_changed = true;
    if (parent_info.number != 0) {
      OUTCOME_TRY(parent_header, block_tree_->getBlockHeader(parent_info.hash));
      OUTCOME_TRY(parent_digest, getBabeDigests(parent_header));
      auto parent_epoch = slotToEpoch(parent_digest.second.slot_number);
      epoch_changed = epoch_number != parent_epoch;
    }
    std::unique_lock lock{indexer_mutex_};
    return config(parent_info, epoch_changed);
  }

  BabeDuration BabeConfigRepositoryImpl::slotDuration() const {
    BOOST_ASSERT_MSG(slot_duration_ != BabeDuration::zero(),
                     "Slot duration is not initialized");
    return slot_duration_;
  }

  EpochLength BabeConfigRepositoryImpl::epochLength() const {
    BOOST_ASSERT_MSG(epoch_length_ != 0, "Epoch length is not initialized");
    return epoch_length_;
  }

  BabeSlotNumber BabeConfigRepositoryImpl::syncEpoch(
      std::function<std::tuple<BabeSlotNumber, bool>()> &&f) {
    if (not is_first_block_finalized_) {
      auto [first_block_slot_number, is_first_block_finalized] = f();
      first_block_slot_number_.emplace(first_block_slot_number);
      is_first_block_finalized_ = is_first_block_finalized;
      SL_TRACE(
          logger_,
          "Epoch beginning is synchronized: first block slot number is {} now",
          first_block_slot_number_.value());
    }
    return first_block_slot_number_.value();
  }

  BabeSlotNumber BabeConfigRepositoryImpl::getCurrentSlot() const {
    return static_cast<BabeSlotNumber>(clock_.now().time_since_epoch()
                                       / slotDuration());
  }

  BabeTimePoint BabeConfigRepositoryImpl::slotStartTime(
      BabeSlotNumber slot) const {
    return clock_.zero() + slot * slotDuration();
  }

  BabeDuration BabeConfigRepositoryImpl::remainToStartOfSlot(
      BabeSlotNumber slot) const {
    auto deadline = slotStartTime(slot);
    auto now = clock_.now();
    if (deadline > now) {
      return deadline - now;
    }
    return BabeDuration{};
  }

  BabeTimePoint BabeConfigRepositoryImpl::slotFinishTime(
      BabeSlotNumber slot) const {
    return slotStartTime(slot + 1);
  }

  BabeDuration BabeConfigRepositoryImpl::remainToFinishOfSlot(
      BabeSlotNumber slot) const {
    return remainToStartOfSlot(slot + 1);
  }

  BabeSlotNumber BabeConfigRepositoryImpl::getFirstBlockSlotNumber() {
    if (first_block_slot_number_.has_value()) {
      return first_block_slot_number_.value();
    }

    return getCurrentSlot();
  }

  EpochNumber BabeConfigRepositoryImpl::slotToEpoch(BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeConfigRepositoryImpl &>(*this).getFirstBlockSlotNumber();
    if (slot > genesis_slot_number) {
      return (slot - genesis_slot_number) / epochLength();
    }
    return 0;
  }

  BabeSlotNumber BabeConfigRepositoryImpl::slotInEpoch(
      BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeConfigRepositoryImpl &>(*this).getFirstBlockSlotNumber();
    if (slot > genesis_slot_number) {
      return (slot - genesis_slot_number) % epochLength();
    }
    return 0;
  }

  void BabeConfigRepositoryImpl::warp(const primitives::BlockInfo &block) {
    std::unique_lock lock{indexer_mutex_};
    indexer_.put(block, {}, true);
  }

  outcome::result<std::shared_ptr<const primitives::BabeConfiguration>>
  BabeConfigRepositoryImpl::config(const primitives::BlockInfo &block,
                                   bool next_epoch) const {
    auto descent = indexer_.descend(block);
    outcome::result<void> cb_res = outcome::success();
    auto cb = [&](std::optional<primitives::BlockInfo> prev,
                  size_t i_first,
                  size_t i_last) {
      cb_res = [&]() -> outcome::result<void> {
        BOOST_ASSERT(i_first >= i_last);
        auto info = descent.path_.at(i_first);
        std::shared_ptr<const primitives::BabeConfiguration> prev_state;
        if (not prev) {
          OUTCOME_TRY(_state, babe_api_->configuration(info.hash));
          auto state = std::make_shared<primitives::BabeConfiguration>(
              std::move(_state));
          BabeIndexedValue value{getConfig(*state), state, state};
          if (info.number == 0) {
            indexer_.put(info, {value, std::nullopt}, true);
          } else {
            std::vector<primitives::BlockInfo> refs;
            while (true) {
              OUTCOME_TRY(header, block_tree_->getBlockHeader(info.hash));
              if (HasBabeConsensusDigest digests{header}) {
                value.next_state = applyDigests(value.config, digests);
                indexer_.put(info, {value, std::nullopt}, true);
                if (not refs.empty()) {
                  indexer_.remove(refs.front());
                }
                break;
              }
              refs.emplace_back(info);
              info = *header.parentInfo();
            }
            std::reverse(refs.begin(), refs.end());
            for (auto &block : refs) {
              indexer_.put(block, {std::nullopt, info, true}, false);
            }
          }
          if (i_first == i_last) {
            return outcome::success();
          }
          prev = info;
          prev_state = *value.next_state;
          --i_first;
        }
        while (true) {
          info = descent.path_.at(i_first);
          OUTCOME_TRY(header, block_tree_->getBlockHeader(info.hash));
          if (HasBabeConsensusDigest digests{header}) {
            if (not prev_state) {
              BOOST_OUTCOME_TRY(prev_state, loadPrev(prev));
            }
            auto state = applyDigests(getConfig(*prev_state), digests);
            BabeIndexedValue value{getConfig(*state), std::nullopt, state};
            indexer_.put(info, {value, prev}, block_tree_->isFinalized(info));
            prev = info;
            prev_state = state;
          } else {
            indexer_.put(info, {std::nullopt, prev, true}, false);
          }
          if (i_first == i_last) {
            break;
          }
          --i_first;
        }
        return outcome::success();
      }();
    };
    auto r = indexer_.search(descent, block, cb);
    OUTCOME_TRY(cb_res);
    if (not r) {
      return Error::NOT_FOUND;
    }
    if (not next_epoch and r->second.value->state) {
      return *r->second.value->state;
    }
    if (next_epoch) {
      OUTCOME_TRY(load(r->first, r->second));
      return *r->second.value->next_state;
    }
    if (not r->second.prev) {
      return Error::PREVIOUS_NOT_FOUND;
    }
    return loadPrev(*r->second.prev);
  }

  std::shared_ptr<primitives::BabeConfiguration>
  BabeConfigRepositoryImpl::applyDigests(
      const primitives::NextConfigDataV1 &config,
      const HasBabeConsensusDigest &digests) const {
    BOOST_ASSERT(digests);
    auto state = std::make_shared<primitives::BabeConfiguration>();
    state->slot_duration = slot_duration_;
    state->epoch_length = epoch_length_;
    if (digests.config) {
      state->leadership_rate = digests.config->ratio;
      state->allowed_slots = digests.config->second_slot;
    } else {
      state->leadership_rate = config.ratio;
      state->allowed_slots = config.second_slot;
    }
    state->authorities = digests.epoch->authorities;
    state->randomness = digests.epoch->randomness;
    return state;
  }

  outcome::result<void> BabeConfigRepositoryImpl::load(
      const primitives::BlockInfo &block,
      blockchain::Indexed<BabeIndexedValue> &item) const {
    if (not item.value->next_state) {
      if (block.number == 0) {
        BOOST_ASSERT(item.value->state);
        item.value->next_state = item.value->state;
      } else {
        OUTCOME_TRY(header, block_tree_->getBlockHeader(block.hash));
        item.value->next_state = applyDigests(item.value->config, {header});
        indexer_.put(block, item, false);
      }
    }
    return outcome::success();
  }

  outcome::result<std::shared_ptr<const primitives::BabeConfiguration>>
  BabeConfigRepositoryImpl::loadPrev(
      const std::optional<primitives::BlockInfo> &prev) const {
    if (not prev) {
      return Error::PREVIOUS_NOT_FOUND;
    }
    auto r = indexer_.get(*prev);
    if (not r) {
      return Error::PREVIOUS_NOT_FOUND;
    }
    if (not r->value) {
      return Error::PREVIOUS_NOT_FOUND;
    }
    OUTCOME_TRY(load(*prev, *r));
    return *r->value->next_state;
  }
}  // namespace kagome::consensus::babe
