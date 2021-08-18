/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_synchronizer_impl.hpp"

#include <random>

#include <boost/assert.hpp>
#include "blockchain/block_tree_error.hpp"
#include "common/visitor.hpp"
#include "network/protocols/sync_protocol.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {
  BabeSynchronizerImpl::BabeSynchronizerImpl(
      std::shared_ptr<network::SyncClientsSet> sync_clients,
      const application::AppConfiguration &app_configuration,
      std::shared_ptr<network::Router> router)
      : sync_clients_{std::move(sync_clients)},
        logger_{log::createLogger("BabeSynchronizer", "babe_synchronizer")},
        app_configuration_(app_configuration),
        router_(std::move(router)) {
    BOOST_ASSERT(sync_clients_);
    BOOST_ASSERT(router_);
  }

  void BabeSynchronizerImpl::request(const primitives::BlockId &from,
                                     const primitives::BlockHash &to,
                                     const libp2p::peer::PeerId &peer_id,
                                     const BlocksHandler &block_list_handler) {
    std::string from_str = visit_in_place(
        from,
        [](const primitives::BlockHash &hash) { return hash.toHex(); },
        [](primitives::BlockNumber number) { return std::to_string(number); });
    logger_->info("Requesting blocks from {} to {}", from_str, to.toHex());

    static std::random_device rd{};
    static std::uniform_int_distribution<primitives::BlocksRequestId> dis{};
    network::BlocksRequest request{
        dis(rd),
        network::BlocksRequest::kBasicAttributes,
        from,
        to,
        network::Direction::ASCENDING,
        static_cast<uint32_t>(app_configuration_.maxBlocksInResponse())};

    return pollClients(request, peer_id, block_list_handler);
  }

  std::shared_ptr<network::SyncProtocolClient>
  BabeSynchronizerImpl::selectNextClient(
      std::unordered_set<std::shared_ptr<network::SyncProtocolClient>>
          &polled_clients) const {
    // we want to ask each client until we get the blocks we lack, but the
    // sync_clients_ set can change between the requests, so we need to keep
    // track of the clients we already asked
    std::shared_ptr<network::SyncProtocolClient> next_client;
    for (const auto &[it, client] : sync_clients_->clients()) {
      if (polled_clients.find(client) == polled_clients.end()) {
        next_client = client;
        polled_clients.insert(client);
        break;
      }
    }

    if (!next_client) {
      // we asked all clients we could, so start over
      polled_clients.clear();
      next_client = sync_clients_->clients().begin()->second;
      polled_clients.insert(next_client);
    }
    return next_client;
  }

  /**
   * Get blocks from resposne
   * @param response containing block data for blocks
   * @return blocks from block data
   */
  boost::optional<std::vector<primitives::Block>> getBlocks(
      const network::BlocksResponse &response) {
    // now we need to check if every block data from response contains header;
    // if any of them does not contain block header, we should proceed to the
    // next client
    std::vector<primitives::Block> blocks;
    for (const auto &block_data : response.blocks) {
      primitives::Block block;
      if (!block_data.header) {
        // that's bad, we can't insert a block, which does not have at
        // least a header
        return boost::none;
      }
      block.header = *block_data.header;

      if (block_data.body) {
        block.body = *block_data.body;
      }

      blocks.push_back(block);
    }
    return blocks;
  }

  void BabeSynchronizerImpl::pollClients(
      network::BlocksRequest request,
      const libp2p::peer::PeerId &peer_id,
      const BlocksHandler &requested_blocks_handler) const {
    auto protocol = router_->getSyncProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide sync protocol");

    protocol->request(
        peer_id,
        std::move(request),
        [wp = weak_from_this(),
         h = requested_blocks_handler](auto &&response_res) mutable {
          if (auto self = wp.lock()) {
            if (not response_res.has_value()) {
              self->logger_->error("Could not sync. Error: {}",
                                   response_res.error().message());
              h(boost::none);
              return;
            }
            auto &response = response_res.value();

            if (response.blocks.empty()) {
              self->logger_->error("Could not sync. Empty response");
              h(boost::none);
              return;
            }

            h(std::cref(response.blocks));
          }
        });

    return;
  }

}  // namespace kagome::consensus
