/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_observer_impl.hpp"

#include <boost/assert.hpp>
#include "blockchain/block_tree_error.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {
  BabeObserverImpl::BabeObserverImpl(
      std::shared_ptr<BlockValidator> validator,
      std::shared_ptr<network::SyncClientsSet> sync_clients,
      std::shared_ptr<blockchain::BlockTree> tree,
      std::shared_ptr<EpochStorage> epoch_storage)
      : validator_{std::move(validator)},
        sync_clients_{std::move(sync_clients)},
        tree_{std::move(tree)},
        epoch_storage_{std::move(epoch_storage)} {
    BOOST_ASSERT(validator_);
    BOOST_ASSERT(sync_clients_);
    BOOST_ASSERT(tree_);
    BOOST_ASSERT(std::all_of(sync_clients_->clients.begin(),
                             sync_clients_->clients.end(),
                             [](const auto &client) { return client; }));
    BOOST_ASSERT(epoch_storage_);
  }

  void BabeObserverImpl::onBlockAnnounce(
      const network::BlockAnnounce &announce) const {
    // maybe later it will be consensus message with a body
    primitives::Block block{announce.header};

    auto epoch_opt = epoch_storage_->getEpoch(announce.header.number);
    if (!epoch_opt) {
      // TODO(akvinikym) 04.09.19: probably some logic should be done here - we
      // cannot validate block without an epoch, but still it needs to be
      // inserted
      return;
    }

    auto validation_res = validator_->validate(block, *epoch_opt);
    if (validation_res) {
      // the block was inserted to the tree by a validator
      return;
    }

    if (validation_res.error() != blockchain::BlockTreeError::NO_PARENT) {
      // pure validation error, the block is invalid
      return;
    }

    // if there's no parent, try to download the lacking blocks; as for now,
    // issue a request to each client one-by-one
    if (sync_clients_->clients.empty()) {
      return;
    }

    auto polled_clients = std::make_shared<decltype(sync_clients_->clients)>();

    // using last_finalized, because if the block, which we want to get, is in
    // non-finalized fork, we are not interested in it; otherwise, it 100% will
    // be a descendant of the last_finalized
    network::BlocksRequest request{network::BlocksRequest::kBasicAttributes,
                                   tree_->getLastFinalized(),
                                   announce.header.parent_hash,
                                   network::Direction::DESCENDING,
                                   boost::none};
  }

  void BabeObserverImpl::pollClients(
      primitives::Block block_to_insert,
      network::BlocksRequest request,
      std::shared_ptr<
          std::unordered_set<std::shared_ptr<network::SyncProtocolClient>>>
          polled_clients) const {
    // we want to ask each client until we get the blocks we lack, but the
    // sync_clients_ set can change between the requests, so we need to keep
    // track of the clients we already asked
    std::shared_ptr<network::SyncProtocolClient> next_client;
    for (const auto &client : sync_clients_->clients) {
      if (polled_clients->find(client) == polled_clients->end()) {
        next_client = client;
        polled_clients->insert(client);
        break;
      }
    }

    if (!next_client) {
      // we asked all clients we could, so nothing can be done
      return;
    }

    next_client->blocksRequest(
        request,
        [self{shared_from_this()},
         block_to_insert{std::move(block_to_insert)},
         request{std::move(request)},
         polled_clients{std::move(polled_clients)}](
            auto &&response_res) mutable {
          if (!response_res) {
            // proceed to the next client
            return self->pollClients(std::move(block_to_insert),
                                     std::move(request),
                                     std::move(polled_clients));
          }
          auto response = std::move(response_res.value());

          // now we need to validate each block from the response, which will
          // also add them to the tree; if any of them fails, we should proceed
          // to the next client
          auto success = false;
          for (const auto &block_data : response.blocks) {
            primitives::Block block;
            if (!block_data.header) {
              // that's bad, we can't insert a block, which does not have at
              // least a header
              break;
            }
            block.header = *block_data.header;

            if (block_data.body) {
              block.body = *block_data.body;
            }

            auto epoch_opt =
                self->epoch_storage_->getEpoch(block.header.number);
            if (!epoch_opt) {
              // not clear, what to do, as in the upper method
              break;
            }

            if (!self->validator_->validate(block, *epoch_opt)) {
              // validation failed, so we cannot proceed
              break;
            }
          }

          if (!success) {
            // proceed to the next client
            return self->pollClients(std::move(block_to_insert),
                                     std::move(request),
                                     std::move(polled_clients));
          }

          // we have inserted all blocks that were needed for the one we
          // received; try to insert it again
          auto insert_res = self->tree_->addBlock(block_to_insert);
          if (!insert_res) {
            // something very bad happened
            return;
          }
        });
  }
}  // namespace kagome::consensus
