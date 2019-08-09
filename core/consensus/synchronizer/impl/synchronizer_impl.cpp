/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/synchronizer/impl/synchronizer_impl.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include "network/types/block_request.hpp"
#include "network/types/block_response.hpp"

namespace kagome::consensus {
  using network::BlockRequest;
  using network::BlockResponse;

  SynchronizerImpl::SynchronizerImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree, common::Logger log)
      : block_tree_{std::move(block_tree)}, log_{std::move(log)} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(log_);
  }

  void SynchronizerImpl::announce(const primitives::Block &block) {}

  void SynchronizerImpl::requestBlocks(const libp2p::peer::PeerInfo &peer,
                                       RequestCallback cb) {
    auto request_id = last_request_id_++;
    BlockRequest request{request_id,
                         BlockRequest::kBasicAttributes,
                         block_tree_->deepestLeaf(),
                         boost::none,
                         network::Direction::DESCENDING,
                         boost::none};
    issued_requests_cbs_.insert({request_id, std::move(cb)});

    peer_client_.requestBlock(std::move(request),
                              [self{shared_from_this()}, request_id](outcome::result<BlockResponse> response_res) {
      if (!response_res) {
        self->log_->error("block request failed: {}", response_res.error().message());

        self->issued_requests_cbs_.erase(request_id);
      }
                              });

    // peer_server.onRequestBlock(BlockRequest)
    // peer_client.requestBlock(BlockRequest, [](BlockResponse) {})

    // peer_client.sendBlock(BlockResponse)

    // peer_client.sendBlock(BlockAnnounce)
    // peer_client.onSendBlock(BlockAnnounce)
  }

  void SynchronizerImpl::requestBlocks(const primitives::BlockHash &hash,
                                       primitives::BlockNumber number,
                                       RequestCallback cb) {}
}  // namespace kagome::consensus
