/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/synchronizer/impl/synchronizer_impl.hpp"

namespace kagome::consensus {
  SynchronizerImpl::SynchronizerImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree, common::Logger log)
      : block_tree_{std::move(block_tree)}, log_{std::move(log)} {}

  void SynchronizerImpl::announce(const primitives::Block &block) {}

  void SynchronizerImpl::requestBlocks(const libp2p::peer::PeerInfo &peer,
                                       RequestCallback cb) {
    // peer_client.onRequestBlock(BlockRequest)
    // peer_client.requestBlock(BlockRequest, [](BlockResponse) {})

    // peer_client.sendBlock(BlockResponse)

    // peer_client.sendBlock(BlockAnnounce)
    // peer_client.onSendBlock(BlockAnnounce)
  }

  void SynchronizerImpl::requestBlocks(const primitives::BlockHash &hash,
                                       primitives::BlockNumber number,
                                       RequestCallback cb) {}
}  // namespace kagome::consensus
