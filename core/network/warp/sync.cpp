/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/warp/sync.hpp"

#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "common/final_action.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "consensus/grandpa/justification_observer.hpp"
#include "network/warp/cache.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"
#include "utils/safe_object.hpp"

namespace kagome::network {
  WarpSync::WarpSync(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<crypto::Hasher> hasher,
      storage::SpacedStorage &db,
      std::shared_ptr<consensus::grandpa::JustificationObserver> grandpa,
      std::shared_ptr<blockchain::BlockStorage> block_storage,
      std::shared_ptr<network::WarpSyncCache> warp_sync_cache,
      std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager,
      std::shared_ptr<consensus::babe::BabeConfigRepository>
          babe_config_repository,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : hasher_{std::move(hasher)},
        grandpa_{std::move(grandpa)},
        block_storage_{std::move(block_storage)},
        warp_sync_cache_{std::move(warp_sync_cache)},
        authority_manager_{std::move(authority_manager)},
        babe_config_repository_{std::move(babe_config_repository)},
        block_tree_{std::move(block_tree)},
        db_{db.getSpace(storage::Space::kDefault)},
        log_{log::createLogger("WarpSync")} {
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
    std::optional<primitives::BlockNumber> min, max;
    common::FinalAction log([&] {
      if (min) {
        SL_INFO(log_, "finalized {}..{}", *min, *max);
      }
    });
    for (size_t i = 0; i < res.proofs.size(); ++i) {
      auto &fragment = res.proofs[i];

      // Calculate and save hash, 'cause it's just received response
      primitives::calculateBlockHash(
          const_cast<primitives::BlockHeader &>(fragment.header), *hasher_);

      primitives::BlockInfo block_info = fragment.header.blockInfo();
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

      auto promise_res =
          std::make_shared<std::promise<outcome::result<void>>>();
      auto res_future = promise_res->get_future();

      grandpa_->verifyJustification(
          fragment.justification, *authorities, promise_res);

      auto result = res_future.get();
      if (result.has_error()) {
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
      if (not min) {
        min = block_info.number;
      }
      max = block_info.number;
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
    babe_config_repository_->warp(op.block_info);
    db_->remove(storage::kWarpSyncOp).value();
  }
}  // namespace kagome::network
