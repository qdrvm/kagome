/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babe_config_repository_impl.hpp"

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "babe.hpp"
#include "babe_digests_util.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/consensus_selector.hpp"
#include "consensus/timeline/slots_util.hpp"
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
  return "unknown error (invalid BabeConfigRepositoryImpl::Error)";
}

namespace kagome::consensus::babe {
  /**
   * If there is more than `kMaxUnindexedBlocksNum` unindexed finalized blocks
   * and last finalized block has state, then babe won't index all of them, but
   * recover with runtime call and latest block with digest.
   */
  constexpr size_t kMaxUnindexedBlocksNum = 10000;

  inline static NextConfigDataV1 getConfig(const BabeConfiguration &state) {
    return {state.leadership_rate, state.allowed_slots};
  }

  BabeConfigRepositoryImpl::BabeConfigRepositoryImpl(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<storage::SpacedStorage> persistent_storage,
      const application::AppConfiguration &app_config,
      EpochTimings &timings,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      LazySPtr<ConsensusSelector> consensus_selector,
      std::shared_ptr<runtime::BabeApi> babe_api,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      LazySPtr<SlotsUtil> slots_util)
      : persistent_storage_(
          persistent_storage->getSpace(storage::Space::kDefault)),
        config_warp_sync_{app_config.syncMethod()
                          == application::SyncMethod::Warp},
        timings_(timings),
        block_tree_(std::move(block_tree)),
        indexer_{
            std::make_shared<storage::MapPrefix>(
                storage::kBabeConfigRepositoryImplIndexerPrefix,
                persistent_storage_),
            block_tree_,
        },
        header_repo_(std::move(header_repo)),
        consensus_selector_(std::move(consensus_selector)),
        babe_api_(std::move(babe_api)),
        trie_storage_(std::move(trie_storage)),
        chain_sub_{chain_events_engine},
        slots_util_(std::move(slots_util)),
        logger_(log::createLogger("BabeConfigRepo", "babe_config_repo")) {
    BOOST_ASSERT(persistent_storage_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(babe_api_ != nullptr);

    SAFE_UNIQUE(indexer_) {
      if (auto r = indexer_.init(); not r) {
        logger_->error("Indexer::init error: {}", r.error());
      }
    };

    app_state_manager.takeControl(*this);
  }

  bool BabeConfigRepositoryImpl::prepare() {
    if (auto res = persistent_storage_->tryGet(storage::kFirstBlockSlot)) {
      if (auto &genesis_slot_raw = res.value()) {
        if (auto res = scale::decode<SlotNumber>(*genesis_slot_raw)) {
          first_block_slot_number_ = res.value();
        } else {
          SL_ERROR(logger_, "genesis slot decode error: {}", res.error());
          std::ignore = persistent_storage_->remove(storage::kFirstBlockSlot);
        }
      }
    } else {
      SL_ERROR(logger_, "genesis slot db read error: {}", res.error());
      return false;
    }

    auto finalized = block_tree_->getLastFinalized();
    auto finalized_header = block_tree_->getBlockHeader(finalized.hash).value();

    SAFE_UNIQUE(indexer_) {
      if (finalized.number - indexer_.last_finalized_indexed_.number
              > kMaxUnindexedBlocksNum
          and trie_storage_->getEphemeralBatchAt(finalized_header.state_root)) {
        warp(indexer_, finalized);
      }

      if (!timings_) {
        auto genesis_res =
            config(indexer_, {block_tree_->getGenesisBlockHash(), 0}, false);
        if (genesis_res.has_value()) {
          auto &genesis = genesis_res.value();
          timings_.init(genesis->slot_duration, genesis->epoch_length);
          SL_DEBUG(logger_,
                   "Timing was initialized: slot is {}ms, epoch is {} slots",
                   timings_.slot_duration.count(),
                   timings_.epoch_length);
        }
      }
    };

    [[maybe_unused]] bool active_ = false;

    auto best = block_tree_->bestBlock();
    auto consensus = consensus_selector_.get()->getProductionConsensus(best);
    if (std::dynamic_pointer_cast<Babe>(consensus)) {
      active_ = true;
      auto res = SAFE_UNIQUE(indexer_) {
        return config(indexer_, best, true);
      };
      if (not res and not config_warp_sync_) {
        SL_ERROR(logger_, "get config at best {} error: {}", best, res.error());
        auto best_header = block_tree_->getBlockHeader(best.hash).value();
        if (not trie_storage_->getEphemeralBatchAt(best_header.state_root)) {
          SL_ERROR(logger_,
                   "warp sync was not completed, restart with \"--sync Warp\"");
        }
        return false;
      }
    }

    chain_sub_.onFinalize([weak{weak_from_this()}]() {
      if (auto self = weak.lock()) {
        auto &indexer_ = self->indexer_;
        SAFE_UNIQUE(indexer_) {
          indexer_.finalize();
        };
      }
    });

    return true;
  }

  outcome::result<std::shared_ptr<const BabeConfiguration>>
  BabeConfigRepositoryImpl::config(const primitives::BlockInfo &parent_info,
                                   EpochNumber epoch_number) const {
    auto epoch_changed = true;
    if (parent_info.number != 0) {
      OUTCOME_TRY(parent_header, block_tree_->getBlockHeader(parent_info.hash));
      OUTCOME_TRY(parent_slot, getSlot(parent_header));
      OUTCOME_TRY(parent_epoch,
                  slots_util_.get()->slotToEpoch(parent_info, parent_slot));
      epoch_changed = epoch_number != parent_epoch;
    }
    return SAFE_UNIQUE(indexer_) {
      return config(indexer_, parent_info, epoch_changed);
    };
  }

  outcome::result<SlotNumber> BabeConfigRepositoryImpl::getFirstBlockSlotNumber(
      const primitives::BlockInfo &parent_info) const {
    auto slot1 = first_block_slot_number_;
    if (not slot1) {
      auto finalized = block_tree_->getLastFinalized();
      OUTCOME_TRY(parent, block_tree_->getBlockHeader(parent_info.hash));
      if (parent.number == 1) {
        BOOST_OUTCOME_TRY(slot1, getSlot(parent));
      } else if (finalized.number != 0) {
        OUTCOME_TRY(hash1, block_tree_->getBlockHash(1));
        if (hash1) {
          OUTCOME_TRY(header1, block_tree_->getBlockHeader(*hash1));
          BOOST_OUTCOME_TRY(slot1, getSlot(header1));
        }
      }
      if (not slot1 and trie_storage_->getEphemeralBatchAt(parent.state_root)) {
        if (auto epoch_res = babe_api_->next_epoch(parent_info.hash);
            epoch_res.has_value()) {
          auto &epoch = epoch_res.value();
          slot1 = epoch.start_slot - epoch.epoch_index * epoch.duration;
        }
      }
      if (not slot1) {
        auto header1 = parent;
        while (header1.number != 1) {
          BOOST_OUTCOME_TRY(header1,
                            block_tree_->getBlockHeader(header1.parent_hash));
        }
        BOOST_OUTCOME_TRY(slot1, getSlot(header1));
      }
      if (finalized.number != 0
          and block_tree_->hasDirectChain(finalized, parent_info)) {
        first_block_slot_number_ = slot1;
        OUTCOME_TRY(persistent_storage_->put(storage::kFirstBlockSlot,
                                             scale::encode(*slot1).value()));
      }
    }
    return slot1.value();
  }

  void BabeConfigRepositoryImpl::warp(Indexer &indexer_,
                                      const primitives::BlockInfo &block) {
    indexer_.put(block, {}, true);
  }

  void BabeConfigRepositoryImpl::warp(const primitives::BlockInfo &block) {
    SAFE_UNIQUE(indexer_) {
      warp(indexer_, block);
    };
  }

  outcome::result<std::shared_ptr<const BabeConfiguration>>
  BabeConfigRepositoryImpl::config(Indexer &indexer_,
                                   const primitives::BlockInfo &block,
                                   bool next_epoch) const {
    auto descent = indexer_.startDescentFrom(block);
    outcome::result<void> cb_res = outcome::success();
    auto cb = [&](std::optional<primitives::BlockInfo> prev,
                  size_t i_first,
                  size_t i_last) {
      cb_res = [&]() -> outcome::result<void> {
        BOOST_ASSERT(i_first >= i_last);
        auto info = descent.path_.at(i_first);
        std::shared_ptr<const BabeConfiguration> prev_state;
        if (not prev) {
          OUTCOME_TRY(_state, babe_api_->configuration(info.hash));
          auto state = std::make_shared<BabeConfiguration>(std::move(_state));
          BabeIndexedValue value{getConfig(*state), state, std::nullopt, state};
          if (info.number != 0) {
            OUTCOME_TRY(next, babe_api_->next_epoch(info.hash));
            BOOST_ASSERT(state->epoch_length == next.duration);
            value.next_state_warp =
                std::make_shared<BabeConfiguration>(BabeConfiguration{
                    state->slot_duration,
                    next.duration,
                    next.leadership_rate,
                    std::move(next.authorities),
                    next.randomness,
                    next.allowed_slots,
                });
            value.next_state = value.next_state_warp;
          }
          indexer_.put(info, {value, std::nullopt}, true);
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
              BOOST_OUTCOME_TRY(prev_state, loadPrev(indexer_, prev));
            }
            auto state = applyDigests(getConfig(*prev_state), digests);
            BabeIndexedValue value{
                getConfig(*state),
                std::nullopt,
                std::nullopt,
                state,
            };
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
      OUTCOME_TRY(load(indexer_, r->first, r->second));
      return *r->second.value->next_state;
    }
    return loadPrev(indexer_, r->second.prev);
  }

  std::shared_ptr<BabeConfiguration> BabeConfigRepositoryImpl::applyDigests(
      const NextConfigDataV1 &config,
      const HasBabeConsensusDigest &digests) const {
    BOOST_ASSERT(digests);
    auto state = std::make_shared<BabeConfiguration>();
    state->slot_duration = timings_.slot_duration;
    state->epoch_length = timings_.epoch_length;
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
      Indexer &indexer_,
      const primitives::BlockInfo &block,
      blockchain::Indexed<BabeIndexedValue> &item) const {
    if (not item.value->next_state) {
      if (block.number == 0) {
        BOOST_ASSERT(item.value->state);
        item.value->next_state = item.value->state;
      } else if (item.value->next_state_warp) {
        item.value->next_state = item.value->next_state_warp;
      } else {
        OUTCOME_TRY(header, block_tree_->getBlockHeader(block.hash));
        item.value->next_state = applyDigests(item.value->config, {header});
        indexer_.put(block, item, false);
      }
    }
    return outcome::success();
  }

  outcome::result<std::shared_ptr<const BabeConfiguration>>
  BabeConfigRepositoryImpl::loadPrev(
      Indexer &indexer_,
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
    OUTCOME_TRY(load(indexer_, *prev, *r));
    return *r->value->next_state;
  }
}  // namespace kagome::consensus::babe
