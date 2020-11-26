/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/syncing_babe.hpp"

namespace kagome::consensus {

  SyncingBabe::SyncingBabe(
      std::shared_ptr<consensus::BlockExecutor> block_executor)
      : block_executor_{std::move(block_executor)} {}

  void SyncingBabe::onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                    const network::BlockAnnounce &announce) {
    block_executor_->processNextBlock(peer_id, announce.header, [](auto &) {});
  }

}  // namespace kagome::consensus
