/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNCHRONIZER_IMPL_HPP
#define KAGOME_SYNCHRONIZER_IMPL_HPP

#include <memory>
#include <unordered_map>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/synchronizer/synchronizer.hpp"
#include "consensus/synchronizer/synchronizer_config.hpp"
#include "network/network_state.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  struct BlockResponse;
  struct BlockRequest;
}  // namespace kagome::network

namespace kagome::consensus {
  class SynchronizerImpl
      : public Synchronizer,
        public std::enable_shared_from_this<SynchronizerImpl> {
   public:
    SynchronizerImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers,
        std::shared_ptr<network::NetworkState> network_state,
        SynchronizerConfig config = {},
        common::Logger log = common::createLogger("Synchronizer"));

    void announce(const primitives::Block &block) override;

    void requestBlocks(const libp2p::peer::PeerInfo &peer,
                       RequestCallback cb) override;

    void requestBlocks(const libp2p::peer::PeerInfo &peer,
                       const primitives::BlockHash &hash,
                       RequestCallback cb) override;

    enum class Error {
      INVALID_BLOCK = 1,
      PEER_RETURNED_NOTHING,
      SYNCHRONIZER_DEAD,
      NO_SUCH_PEER
    };

   private:
    /**
     * Internal method for requesting blocks
     */
    void requestBlocks(network::BlockRequest request,
                       const libp2p::peer::PeerInfo &peer,
                       RequestCallback cb) const;

    /**
     * Handle a response for our request
     */
    void handleBlocksResponse(
        const outcome::result<network::BlockResponse> &response_res,
        const RequestCallback &cb) const;

    /**
     * Process a BlockRequest
     * @param request to be processed
     * @return a response to the request
     */
    outcome::result<network::BlockResponse> processRequest(
        const network::BlockRequest &request) const;

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers_;
    std::shared_ptr<network::NetworkState> network_state_;
    SynchronizerConfig config_;
    common::Logger log_;

    primitives::BlockRequestId last_request_id_ = 0;
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, SynchronizerImpl::Error)

#endif  // KAGOME_SYNCHRONIZER_IMPL_HPP
