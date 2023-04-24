/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/warp/sync.hpp"

#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "consensus/grandpa/justification_observer.hpp"
#include "network/warp/cache.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::network {
  WarpSync::WarpSync(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<crypto::Hasher> hasher,
      storage::SpacedStorage &db,
      std::shared_ptr<consensus::grandpa::JustificationObserver> grandpa,
      std::shared_ptr<blockchain::BlockStorage> block_storage,
      std::shared_ptr<network::WarpSyncCache> warp_sync_cache,
      std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : hasher_{std::move(hasher)},
        grandpa_{std::move(grandpa)},
        block_storage_{std::move(block_storage)},
        warp_sync_cache_{std::move(warp_sync_cache)},
        authority_manager_{std::move(authority_manager)},
        block_tree_{std::move(block_tree)},
        db_{db.getSpace(storage::Space::kDefault)} {
    app_state_manager.atLaunch([this] {
      start();
      return true;
    });
  }

  void WarpSync::start() {
    if (auto old = db_->tryGet(storage::kWarpSyncOp).value()) {
      applyInner(scale::decode<Op>(*old).value());
    }
  }

  std::optional<primitives::BlockInfo> WarpSync::request() const {
    if (done_) {
      return std::nullopt;
    }
    return block_tree_->getLastFinalized();
  }

  void WarpSync::onResponse(const WarpSyncProof &res) {
    done_ = true;
    if (res.proofs.empty()) {
      return;
    }
    for (size_t i = 0; i < res.proofs.size(); ++i) {
      auto &fragment = res.proofs[i];
      primitives::BlockInfo block_info{
          hasher_->blake2b_256(scale::encode(fragment.header).value()),
          fragment.header.number,
      };
      if (fragment.justification.block_info != block_info) {
        return;
      }
      consensus::grandpa::HasAuthoritySetChange change{fragment.header};
      if (not change.scheduled and i != res.proofs.size() - 1) {
        return;
      }
      auto authorities =
          authority_manager_->authorities(block_tree_->getLastFinalized(), true)
              .value();
      if (not grandpa_->verifyJustification(fragment.justification,
                                            *authorities)) {
        return;
      }
      Op op{
          block_info,
          fragment.header,
          fragment.justification,
          *authorities,
      };
      db_->put(storage::kWarpSyncOp, scale::encode(op).value()).value();
      applyInner(op);
    }
    if (not res.is_finished) {
      done_ = false;
    }
  }

  void WarpSync::applyInner(const Op &op) {
    block_storage_
        ->putJustification(
            {common::Buffer{scale::encode(op.justification).value()}},
            op.block_info.hash)
        .value();
    block_storage_->putBlockHeader(op.header).value();
    block_storage_->assignNumberToHash(op.block_info).value();
    block_storage_->setBlockTreeLeaves({op.block_info.hash}).value();
    warp_sync_cache_->warp(op.block_info);
    authority_manager_->warp(op.block_info, op.header, op.authorities);
    block_tree_->warp(op.block_info);
    db_->remove(storage::kWarpSyncOp).value();
  }
}  // namespace kagome::network
