/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNC_PROTOCOL_OBSERVER_IMPL
#define KAGOME_SYNC_PROTOCOL_OBSERVER_IMPL

#include "network/sync_protocol_observer.hpp"

#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_info.hpp>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "network/types/own_peer_info.hpp"
#include "primitives/common.hpp"

namespace kagome::network {

  class SyncProtocolObserverImpl
      : public SyncProtocolObserver,
        public std::enable_shared_from_this<SyncProtocolObserverImpl> {
   public:
    /// how much blocks we can send at once
    static const size_t maxRequestBlocks = 128u;

    enum class Error { DUPLICATE_REQUEST_ID = 1 };

    SyncProtocolObserverImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers);

    ~SyncProtocolObserverImpl() override = default;

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

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers_;
    mutable std::unordered_set<primitives::BlocksRequestId> requested_ids_;
    common::Logger log_;
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, SyncProtocolObserverImpl::Error);

#endif  // KAGOME_SYNC_PROTOCOL_OBSERVER_IMPL
