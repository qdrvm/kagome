/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/synchronizer/impl/synchronizer_impl.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include "blockchain/block_tree_error.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/block_request.hpp"
#include "network/types/block_response.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, SynchronizerImpl::Error, e) {
  using E = kagome::consensus::SynchronizerImpl::Error;
  switch (e) {
    case E::INVALID_BLOCK:
      return "peer has sent an invalid or incomplete block";
    case E::PEER_RETURNED_NOTHING:
      return "peer has not sent any blocks in response; either it does not "
             "have a required hash or it is in non-finalized fork";
    case E::SYNCHRONIZER_DEAD:
      return "instance of synchronizer, which is called, is dead";
    case E::NO_SUCH_PEER:
      return "no peer with such ID found";
  }
  return "unknown error";
}

namespace kagome::consensus {
  using network::BlockRequest;
  using network::BlockResponse;

  SynchronizerImpl::SynchronizerImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers,
      std::shared_ptr<network::NetworkState> network_state,
      SynchronizerConfig config,
      common::Logger log)
      : block_tree_{std::move(block_tree)},
        blocks_headers_{std::move(blocks_headers)},
        network_state_{std::move(network_state)},
        config_{config},
        log_{std::move(log)} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(blocks_headers_);
    BOOST_ASSERT(network_state_);
    BOOST_ASSERT(log_);

    network_state_->peer_server->onBlocksRequest(
        [self{weak_from_this()}](
            const BlockRequest &request) -> outcome::result<BlockResponse> {
          if (self.expired()) {
            return Error::SYNCHRONIZER_DEAD;
          }
          return self.lock()->processRequest(request);
        });
  }

  void SynchronizerImpl::announce(const primitives::BlockHeader &block_header) {
    auto announce = network::BlockAnnounce{block_header};
    for (const auto &peer_client : network_state_->peer_clients) {
      peer_client.second->blockAnnounce(
          announce, [self{shared_from_this()}, peer_client](auto &&res) {
            if (!res) {
              self->log_->error("cannot send a block announce to peer {}: {}",
                                peer_client.first.toBase58(),
                                res.error().message());
            }
          });
    }
  }

  void SynchronizerImpl::requestBlocks(const libp2p::peer::PeerInfo &peer,
                                       RequestCallback cb) {
    auto request_id = last_request_id_++;
    BlockRequest request{request_id,
                         BlockRequest::kBasicAttributes,
                         block_tree_->deepestLeaf(),
                         boost::none,
                         network::Direction::DESCENDING,
                         boost::none};
    requestBlocks(std::move(request), peer, std::move(cb));
  }

  void SynchronizerImpl::requestBlocks(const libp2p::peer::PeerInfo &peer,
                                       const primitives::BlockHash &hash,
                                       RequestCallback cb) {
    auto request_id = last_request_id_++;
    // using last_finalized, because if the block, which we want to get, is in
    // non-finalized fork, we are not interested in it; otherwise, it 100% will
    // be a descendant of the last_finalized
    BlockRequest request{request_id,
                         BlockRequest::kBasicAttributes,
                         block_tree_->getLastFinalized(),
                         hash,
                         network::Direction::DESCENDING,
                         boost::none};
    requestBlocks(std::move(request), peer, std::move(cb));
  }

  void SynchronizerImpl::requestBlocks(network::BlockRequest request,
                                       const libp2p::peer::PeerInfo &peer,
                                       RequestCallback cb) const {
    auto peer_client_it = network_state_->peer_clients.find(peer.id);
    if (peer_client_it == network_state_->peer_clients.end()) {
      log_->info("no peer with id {}", peer.id.toBase58());
      return cb(Error::NO_SUCH_PEER);
    }

    peer_client_it->second->blocksRequest(
        std::move(request),
        [self{shared_from_this()},
         cb = std::move(cb)](auto &&block_response_res) mutable {
          self->handleBlocksResponse(
              std::forward<decltype(block_response_res)>(block_response_res),
              cb);
        });
  }

  void SynchronizerImpl::handleBlocksResponse(
      const outcome::result<network::BlockResponse> &response_res,
      const RequestCallback &cb) const {
    if (!response_res) {
      return cb(response_res.error());
    }

    for (auto &block_data : response_res.value().blocks) {
      auto block_opt = block_data.toBlock();
      if (!block_opt) {
        // there is no sense in continuing: blocks are coming in
        // sequential order, and if one of them is skipped, we will
        // not be able to insert any further
        return cb(Error::INVALID_BLOCK);
      }
      auto block_insert_res = block_tree_->addBlock(std::move(*block_opt));
      if (!block_insert_res
          && block_insert_res.error()
                 != blockchain::BlockTreeError::BLOCK_EXISTS) {
        // if such block already exists, it's OK
        return cb(block_insert_res.error());
      }
    }

    cb(outcome::success());
  }

  outcome::result<network::BlockResponse> SynchronizerImpl::processRequest(
      const BlockRequest &request) const {
    BlockResponse response{request.id};

    // firstly, check if we have both "from" & "to" blocks (if set)
    auto from_hash_res = blocks_headers_->getHashById(request.from);
    if (!from_hash_res) {
      log_->info("cannot find a requested block with id {}", request.from);
      return response;
    }
    auto from_hash = from_hash_res.value();

    // secondly, retrieve hashes of blocks the other peer is interested in
    auto ascending_direction =
        request.direction == network::Direction::ASCENDING;
    blockchain::BlockTree::BlockHashVecRes chain_hash_res{{}};
    if (!request.to) {
      // if there's no "stop" block, get as many as possible
      chain_hash_res = block_tree_->getChainByBlock(
          from_hash, ascending_direction, config_.max_request_blocks);
    } else {
      // else, both blocks are specified
      chain_hash_res = block_tree_->getChainByBlocks(
          ascending_direction ? *request.to : from_hash,
          ascending_direction ? from_hash : *request.to);
    }
    if (!chain_hash_res) {
      log_->error("cannot retrieve a chain of blocks: {}",
                  chain_hash_res.error().message());
      return response;
    }
    auto hash_chain = std::move(chain_hash_res.value());

    // thirdly, fill the resulting response with data, which we were asked for
    // TODO(akvinikym): understand, where to take receipt and message_queue
    auto header_needed =
        request.attributeIsSet(network::BlockAttributesBits::HEADER);
    auto body_needed =
        request.attributeIsSet(network::BlockAttributesBits::BODY);
    auto justification_needed =
        request.attributeIsSet(network::BlockAttributesBits::JUSTIFICATION);
    for (const auto &hash : hash_chain) {
      auto &new_block = response.blocks.emplace_back(network::BlockData{hash});

      if (header_needed) {
        auto header_res = blocks_headers_->getBlockHeader(hash);
        if (header_res) {
          // normal situation - we have got part of the blocks, for example,
          // only header or body
          new_block.header = std::move(header_res.value());
        }
      }
      if (body_needed) {
        auto body_res = block_tree_->getBlockBody(hash);
        if (body_res) {
          new_block.body = std::move(body_res.value());
        }
      }
      if (justification_needed) {
        auto justification_res = block_tree_->getBlockJustification(hash);
        if (justification_res) {
          new_block.justification = std::move(justification_res.value());
        }
      }
    }

    return response;
  }
}  // namespace kagome::consensus
