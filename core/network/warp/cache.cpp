/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/warp/cache.hpp"

#include <boost/endian/buffers.hpp>
#include <boost/endian/conversion.hpp>

#include "blockchain/impl/storage_util.hpp"
#include "common/final_action.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "storage/predefined_keys.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, WarpSyncCache::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::NOT_FINALIZED:
      return "Requested block is not finalized";
    case E::NOT_IN_CHAIN:
      return "Requested block is not in chain";
  }
  return "unknown error (invalid WarpSyncCache::Error";
}

namespace kagome::network {
  using consensus::grandpa::HasAuthoritySetChange;

  /**
   * The maximum size in bytes of the `WarpSyncProof`.
   * https://github.com/paritytech/substrate/blob/86c6bb9614c437b63f3dbd2afddef52f32af7866/client/finality-grandpa/src/warp_proof.rs#L57
   */
  constexpr size_t kMaxFragmentsSize = (8 << 20) + 50;

  inline common::Buffer toKey(primitives::BlockNumber i) {
    common::Buffer res(sizeof(i));
    boost::endian::store_big_u32(res.data(), i);
    return res;
  }

  inline primitives::BlockNumber fromKey(common::BufferView key) {
    return boost::endian::load_big_u32(key.data());
  }

  WarpSyncCache::WarpSyncCache(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repository,
      std::shared_ptr<storage::SpacedStorage> db,
      std::shared_ptr<primitives::events::ChainSubscriptionEngine>
          chain_sub_engine)
      : block_tree_{std::move(block_tree)},
        block_repository_{std::move(block_repository)},
        db_prefix_{
            storage::kWarpSyncCacheBlocksPrefix,
            db->getSpace(storage::Space::kDefault),
        },
        log_{log::createLogger("WarpSyncCache", "warp_sync_protocol")} {
    app_state_manager.atLaunch([=, this]() mutable {
      auto r = start(std::move(chain_sub_engine));
      if (not r) {
        SL_WARN(log_, "start error {}", r.error());
        // TODO(turuslan): #1536 warp sync forced change
        return true;
      }
      return true;
    });
  }

  outcome::result<WarpSyncProof> WarpSyncCache::getProof(
      const primitives::BlockHash &after_hash) const {
    OUTCOME_TRY(after_number, block_repository_->getNumberByHash(after_hash));
    auto finalized = block_tree_->getLastFinalized();
    if (after_number > finalized.number) {
      return Error::NOT_FINALIZED;
    }
    OUTCOME_TRY(expected_hash,
                block_repository_->getHashByNumber(after_number));
    if (after_hash != expected_hash) {
      return Error::NOT_IN_CHAIN;
    }
    WarpSyncProof res;
    res.is_finished = true;
    auto size_limit = kMaxFragmentsSize;
    primitives::BlockNumber last_proof = 0;
    auto cursor = db_prefix_.cursor();
    OUTCOME_TRY(cursor->seek(toKey(after_number + 1)));
    while (cursor->isValid()) {
      auto number = fromKey(*cursor->key());
      OUTCOME_TRY(hash, primitives::BlockHash::fromSpan(*cursor->value()));
      OUTCOME_TRY(header, block_repository_->getBlockHeader(hash));
      HasAuthoritySetChange change{header};
      if (not change.scheduled) {
        break;
      }
      OUTCOME_TRY(raw_justification, block_tree_->getBlockJustification(hash));
      OUTCOME_TRY(justification,
                  scale::decode<consensus::grandpa::GrandpaJustification>(
                      raw_justification.data));
      WarpSyncFragment fragment{std::move(header), std::move(justification)};
      auto fragment_size = scale::encode(fragment).value().size();
      if (fragment_size > size_limit) {
        res.is_finished = false;
        break;
      }
      size_limit -= fragment_size;
      res.proofs.emplace_back(std::move(fragment));
      last_proof = number;
      OUTCOME_TRY(cursor->next());
    }
    if (res.is_finished && finalized.number > last_proof) {
      OUTCOME_TRY(header, block_repository_->getBlockHeader(finalized.hash));
      OUTCOME_TRY(raw_justification,
                  block_tree_->getBlockJustification(finalized.hash));
      OUTCOME_TRY(justification,
                  scale::decode<consensus::grandpa::GrandpaJustification>(
                      raw_justification.data));
      res.proofs.emplace_back(
          WarpSyncFragment{std::move(header), std::move(justification)});
    }
    return res;
  }

  void WarpSyncCache::warp(const primitives::BlockInfo &block) {
    db_prefix_.put(toKey(block.number), block.hash).value();
    cache_next_ = block.number + 1;
  }

  outcome::result<void> WarpSyncCache::cacheMore(
      primitives::BlockNumber finalized) {
    if (not started_.load()) {
      return outcome::success();
    }
    if (bool old = false; not caching_.compare_exchange_strong(old, true)) {
      return outcome::success();
    }
    common::FinalAction unlock([&] { caching_.store(false); });
    for (; cache_next_ <= finalized; ++cache_next_) {
      OUTCOME_TRY(hash, block_repository_->getHashByNumber(cache_next_));
      OUTCOME_TRY(header, block_repository_->getBlockHeader(hash));
      if (HasAuthoritySetChange change{header}) {
        if (change.scheduled) {
          OUTCOME_TRY(raw_justification,
                      block_tree_->getBlockJustification(hash));
          OUTCOME_TRY(scale::decode<consensus::grandpa::GrandpaJustification>(
              raw_justification.data));
        }
        OUTCOME_TRY(db_prefix_.put(toKey(cache_next_), hash));
      }
    }
    return outcome::success();
  }

  outcome::result<void> WarpSyncCache::start(
      std::shared_ptr<primitives::events::ChainSubscriptionEngine>
          chain_sub_engine) {
    auto cursor = db_prefix_.cursor();
    OUTCOME_TRY(cursor->seekLast());
    while (cursor->isValid()) {
      auto key = *cursor->key();
      auto number = fromKey(key);
      OUTCOME_TRY(expected_hash,
                  primitives::BlockHash::fromSpan(*cursor->value()));
      if (auto hash_res = block_repository_->getHashByNumber(number)) {
        if (hash_res.value() == expected_hash) {
          cache_next_ = number + 1;
          break;
        }
      }
      OUTCOME_TRY(cursor->prev());
      OUTCOME_TRY(db_prefix_.remove(key));
    }
    started_.store(true);
    OUTCOME_TRY(cacheMore(block_tree_->getLastFinalized().number));

    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        chain_sub_engine);
    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kFinalizedHeads);
    auto on_finalize = [weak = weak_from_this()](
                           subscription::SubscriptionSetId,
                           auto &&,
                           primitives::events::ChainEventType,
                           const primitives::events::ChainEventParams &event) {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      auto r = self->cacheMore(
          boost::get<primitives::events::HeadsEventParams>(event).get().number);
      if (not r) {
        SL_WARN(self->log_, "cacheMore error {}", r.error());
      }
    };
    chain_sub_->setCallback(std::move(on_finalize));

    return outcome::success();
  }
}  // namespace kagome::network
