/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_synchronizer_impl.hpp"

#include <random>

#include <boost/assert.hpp>
#include "blockchain/block_tree_error.hpp"
#include "common/visitor.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {
  BabeSynchronizerImpl::BabeSynchronizerImpl(
      std::shared_ptr<network::SyncClientsSet> sync_clients)
      : sync_clients_{std::move(sync_clients)},
        logger_{common::createLogger("BabeSynchronizer")} {
    BOOST_ASSERT(sync_clients_);
    BOOST_ASSERT(std::all_of(sync_clients_->clients.begin(),
                             sync_clients_->clients.end(),
                             [](const auto &client) { return client; }));
  }

  void BabeSynchronizerImpl::request(const primitives::BlockId &from,
                                     const primitives::BlockHash &to,
                                     const BlocksHandler &block_list_handler) {
    std::string from_str = visit_in_place(
        from,
        [](const primitives::BlockHash &hash) { return hash.toHex(); },
        [](primitives::BlockNumber number) { return std::to_string(number); });
    logger_->info("Requesting blocks from {} to {}", from_str, to.toHex());

    static std::random_device rd{};
    static std::uniform_int_distribution<primitives::BlocksRequestId> dis{};
    network::BlocksRequest request{dis(rd),
                                   network::BlocksRequest::kBasicAttributes,
                                   from,
                                   to,
                                   network::Direction::DESCENDING,
                                   boost::none};

    return pollClients(request, {}, block_list_handler);
  }

  void BabeSynchronizerImpl::pollClients(
      network::BlocksRequest request,
      std::unordered_set<std::shared_ptr<network::SyncProtocolClient>>
          &&polled_clients,
      const BlocksHandler &requested_blocks_handler) const {
    // we want to ask each client until we get the blocks we lack, but the
    // sync_clients_ set can change between the requests, so we need to keep
    // track of the clients we already asked
    std::shared_ptr<network::SyncProtocolClient> next_client;
    for (const auto &client : sync_clients_->clients) {
      if (polled_clients.find(client) == polled_clients.end()) {
        next_client = client;
        polled_clients.insert(client);
        break;
      }
    }

    if (!next_client) {
      // we asked all clients we could, so start over
      polled_clients.clear();
      next_client = *sync_clients_->clients.begin();
      polled_clients.insert(next_client);
    }

    next_client->blocksRequest(
        request,
        [self{shared_from_this()},
         request{std::move(request)},
         polled_clients{std::move(polled_clients)},
         requested_blocks_handler{std::move(requested_blocks_handler)}](
            auto &&response_res) mutable {
          if (!response_res or response_res.value().blocks.empty()) {
            // proceed to the next client
            return self->pollClients(std::move(request),
                                     std::move(polled_clients),
                                     requested_blocks_handler);
          }
          auto response = std::move(response_res.value());

          // now we need to validate each block from the response, which will
          // also add them to the tree; if any of them fails, we should proceed
          // to the next client
          auto success = true;
          std::vector<primitives::Block> blocks;
          for (const auto &block_data : response.blocks) {
            primitives::Block block;
            if (!block_data.header) {
              // that's bad, we can't insert a block, which does not have at
              // least a header
              success = false;
              break;
            }
            block.header = *block_data.header;

            if (block_data.body) {
              block.body = *block_data.body;
            }

            blocks.push_back(block);
          }

          if (!success) {
            // proceed to the next client
            return self->pollClients(std::move(request),
                                     std::move(polled_clients),
                                     std::move(requested_blocks_handler));
          }

          requested_blocks_handler(blocks);
        });
  }
}  // namespace kagome::consensus
