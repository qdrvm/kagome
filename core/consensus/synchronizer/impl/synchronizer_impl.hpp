/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNCHRONIZER_IMPL_HPP
#define KAGOME_SYNCHRONIZER_IMPL_HPP

#include <memory>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/synchronizer/synchronizer.hpp"
#include "consensus/synchronizer/synchronizer_config.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {
  class SynchronizerImpl
      : public Synchronizer,
        public std::enable_shared_from_this<SynchronizerImpl> {
    using BlocksResponse = network::BlocksResponse;
    using BlocksRequest = network::BlocksRequest;

   public:
    SynchronizerImpl(
        libp2p::peer::PeerInfo peer_info,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers,
        SynchronizerConfig config = {},
        common::Logger log = common::createLogger("Synchronizer"));

    ~SynchronizerImpl() override = default;

    void announce(const primitives::BlockHeader &block_header) override;

    void blocksRequest(
        const BlocksRequest &request,
        std::function<void(outcome::result<BlocksResponse>)> cb) const override;

    outcome::result<BlocksResponse> onBlocksRequest(
        const BlocksRequest &request) const override;

   private:
    blockchain::BlockTree::BlockHashVecRes retrieveRequestedHashes(
        const network::BlocksRequest &request,
        const primitives::BlockHash &from_hash) const;

    void fillBlocksResponse(
        const network::BlocksRequest &request,
        network::BlocksResponse &response,
        const std::vector<primitives::BlockHash> &hash_chain) const;

    libp2p::peer::PeerInfo peer_info_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers_;
    SynchronizerConfig config_;
    common::Logger log_;

    primitives::BlocksRequestId last_request_id_ = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_SYNCHRONIZER_IMPL_HPP
