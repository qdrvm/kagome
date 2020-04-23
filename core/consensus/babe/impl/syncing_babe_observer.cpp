/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/syncing_babe_observer.hpp"

namespace kagome::consensus {

  SyncingBabeObserver::SyncingBabeObserver(
      std::shared_ptr<consensus::BlockExecutor> block_executor)
      : block_executor_{std::move(block_executor)} {}

  void SyncingBabeObserver::onBlockAnnounce(
      const network::BlockAnnounce &announce) {
    block_executor_->processNextBlock(announce.header, [](auto &) {});
  }

}  // namespace kagome::consensus
