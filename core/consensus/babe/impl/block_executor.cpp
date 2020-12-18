/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_executor.hpp"

#include <chrono>
#include <libp2p/peer/peer_id.hpp>

#include "blockchain/block_tree_error.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "scale/scale.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BlockExecutor::Error, e) {
  using E = kagome::consensus::BlockExecutor::Error;
  switch (e) {
    case E::INVALID_BLOCK:
      return "Invalid block";
  }
  return "Unknown error";
}

namespace kagome::consensus {

  BlockExecutor::BlockExecutor(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> core,
      std::shared_ptr<primitives::BabeConfiguration> configuration,
      std::shared_ptr<consensus::BabeSynchronizer> babe_synchronizer,
      std::shared_ptr<consensus::BlockValidator> block_validator,
      std::shared_ptr<consensus::JustificationValidator>
          justification_validator,
      std::shared_ptr<consensus::EpochStorage> epoch_storage,
      std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<authority::AuthorityUpdateObserver>
          authority_update_observer,
      SlotsStrategy slots_strategy)
      : sync_state_(kReadyState),
        block_tree_{std::move(block_tree)},
        core_{std::move(core)},
        genesis_configuration_{std::move(configuration)},
        babe_synchronizer_{std::move(babe_synchronizer)},
        block_validator_{std::move(block_validator)},
        justification_validator_{std::move(justification_validator)},
        epoch_storage_{std::move(epoch_storage)},
        tx_pool_{std::move(tx_pool)},
        hasher_{std::move(hasher)},
        authority_update_observer_{std::move(authority_update_observer)},
        slots_strategy_{slots_strategy},
        logger_{common::createLogger("BlockExecutor")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(core_ != nullptr);
    BOOST_ASSERT(genesis_configuration_ != nullptr);
    BOOST_ASSERT(babe_synchronizer_ != nullptr);
    BOOST_ASSERT(block_validator_ != nullptr);
    BOOST_ASSERT(epoch_storage_ != nullptr);
    BOOST_ASSERT(tx_pool_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(authority_update_observer_ != nullptr);
    BOOST_ASSERT(justification_validator_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
  }

  void BlockExecutor::processNextBlock(
      const libp2p::peer::PeerId &peer_id,
      const primitives::BlockHeader &header,
      const std::function<void(const primitives::BlockHeader &)>
          &new_block_handler) {
    auto block_hash = hasher_->blake2b_256(scale::encode(header).value());

    // insert block_header if it is missing
    if (not block_tree_->getBlockHeader(block_hash)) {
      new_block_handler(header);
      logger_->info("Received block header. Number: {}, Hash: {}",
                    header.number,
                    block_hash.toHex());

      auto [_, babe_header] = getBabeDigests(header).value();

      if (not block_tree_->getBlockHeader(header.parent_hash)) {
        if (sync_state_ == kReadyState) {
          /// We don't have past block, it means we have a gap and must sync
          sync_state_ = kSyncState;
          const auto &[last_number, last_hash] =
              block_tree_->getLastFinalized();
          // we should request blocks between last finalized one and received
          // block
          requestBlocks(
              last_hash, block_hash, peer_id, [wself{weak_from_this()}] {
                if (auto self = wself.lock()) self->sync_state_ = kReadyState;
              });
        }
      } else {
        requestBlocks(header.parent_hash, block_hash, peer_id, [] {});
      }
    }
  }

  void BlockExecutor::requestBlocks(const libp2p::peer::PeerId &peer_id,
                                    const primitives::BlockHeader &new_header,
                                    std::function<void()> &&next) {
    const auto &[last_number, last_hash] = block_tree_->getLastFinalized();
    auto new_block_hash =
        hasher_->blake2b_256(scale::encode(new_header).value());
    BOOST_ASSERT(new_header.number >= last_number);
    auto [_, babe_header] = getBabeDigests(new_header).value();
    return requestBlocks(last_hash, new_block_hash, peer_id, std::move(next));
  }

  void BlockExecutor::requestBlocks(const primitives::BlockHash &from,
                                    const primitives::BlockHash &to,
                                    const libp2p::peer::PeerId &peer_id,
                                    std::function<void()> &&next) {
    babe_synchronizer_->request(
        from,
        to,
        peer_id,
        [self_wp{weak_from_this()}, next(std::move(next)), to, from, peer_id](
            auto blocks_res) mutable {
          auto self = self_wp.lock();
          if (!self || !blocks_res) {
            next();
            return;
          }

          const auto &blocks = blocks_res->get();
          bool sync_complete = false;
          const primitives::BlockHash *last_received_hash = nullptr;
          if (blocks.empty()) {
            self->logger_->warn("Received empty list of blocks");
            sync_complete = true;
          } else if (blocks.front().header && blocks.back().header) {
            auto &first_received_hash = blocks.front().hash;
            last_received_hash = &blocks.back().hash;
            self->logger_->info("Received blocks from: {}, to {}, count {}",
                                first_received_hash.toHex(),
                                last_received_hash->toHex(),
                                blocks.size());
          } else {
            self->logger_->warn("Blocks with empty headers detected.");
            sync_complete = true;
          }
          for (const auto &block : blocks) {
            const auto &block_hash = block.hash;
            if (to == block_hash) {
              sync_complete = true;
            }
            if (auto apply_res = self->applyBlock(block); not apply_res) {
              if (apply_res
                  == outcome::failure(
                      blockchain::BlockTreeError::BLOCK_EXISTS)) {
                continue;
              }
              self->logger_->warn(
                  "Could not apply block during synchronizing slots.Error: {}",
                  apply_res.error().message());
              sync_complete = true;
              break;
            }
          }
          if (sync_complete)
            next();
          else {
            self->logger_->info("Request next page of blocks: from {}, to {}",
                                last_received_hash->toHex(),
                                to.toHex());
            self->requestBlocks(
                *last_received_hash, to, peer_id, std::move(next));
          }
        });
  }

  outcome::result<void> BlockExecutor::applyBlock(
      const primitives::BlockData &b) {
    if (!b.header) {
      logger_->warn("Skipping blockwithout header.");
      return Error::INVALID_BLOCK;
    }

    /// TODO(iceseer): remove copy.
    primitives::Block block;
    block.header = *b.header;
    if (b.body) block.body = *b.body;
    // get current time to measure performance if block execution
    auto t_start = std::chrono::high_resolution_clock::now();

    auto block_hash = hasher_->blake2b_256(scale::encode(block.header).value());

    // check if block body already exists. If so, do not apply
    if (block_tree_->getBlockBody(block_hash)) {
      OUTCOME_TRY(block_tree_->addExistingBlock(block_hash, block.header));

      logger_->debug("Skipping existed block number: {}, hash: {}",
                     block.header.number,
                     block_hash.toHex());
      return blockchain::BlockTreeError::BLOCK_EXISTS;
    }
    logger_->info("Applying block number: {}, hash: {}",
                  block.header.number,
                  block_hash.toHex());

    OUTCOME_TRY(babe_digests, getBabeDigests(block.header));

    const auto &[seal, babe_header] = babe_digests;

    {
      // add information about epoch to epoch storage
      if (slots_strategy_ == SlotsStrategy::FromUnixEpoch
          and block.header.number == 1) {
        OUTCOME_TRY(epoch_storage_->setLastEpoch(LastEpochDescriptor{
            .epoch_number = 0,
            .start_slot = babe_header.slot_number,
            .epoch_duration = genesis_configuration_->epoch_length,
            .starting_slot_finish_time =
                BabeTimePoint{(babe_header.slot_number + 1)
                              * genesis_configuration_->slot_duration}}));
      }
    }

    EpochIndex epoch_index;
    BabeSlotNumber slot_in_epoch;
    switch (slots_strategy_) {
      case SlotsStrategy::FromZero:
        epoch_index =
            babe_header.slot_number / genesis_configuration_->epoch_length;
        slot_in_epoch = 0ull;
        break;
      case SlotsStrategy::FromUnixEpoch:
        OUTCOME_TRY(last_epoch, epoch_storage_->getLastEpoch());
        const auto last_epoch_start_slot = last_epoch.start_slot;
        const auto last_epoch_index = last_epoch.epoch_number;
        const auto slot_dif = babe_header.slot_number - last_epoch_start_slot;

        epoch_index =
            last_epoch_index + slot_dif / genesis_configuration_->epoch_length;
        slot_in_epoch = slot_dif % genesis_configuration_->epoch_length;
        break;
    }

    OUTCOME_TRY(this_block_epoch_descriptor,
                epoch_storage_->getEpochDescriptor(epoch_index));

    auto threshold = calculateThreshold(genesis_configuration_->leadership_rate,
                                        this_block_epoch_descriptor.authorities,
                                        babe_header.authority_index);

    // update authorities and randomnesss
    if (0ull == slot_in_epoch) {
      if (auto next_epoch_digest_res = getNextEpochDigest(block.header)) {
        logger_->info("Got next epoch digest for epoch: {}", epoch_index);
        epoch_storage_
            ->addEpochDescriptor(epoch_index + 1, next_epoch_digest_res.value())
            .value();
      } else {
        logger_->error("Failed to get next epoch digest for epoch: {}",
                       epoch_index);
      }
    }

    OUTCOME_TRY(block_validator_->validateHeader(
        block.header,
        epoch_index,
        this_block_epoch_descriptor.authorities[babe_header.authority_index].id,
        threshold,
        this_block_epoch_descriptor.randomness));

    auto block_without_seal_digest = block;

    // block should be applied without last digest which contains the seal
    block_without_seal_digest.header.digest.pop_back();
    // apply block
    OUTCOME_TRY(core_->execute_block(block_without_seal_digest));

    // add block header if it does not exist
    OUTCOME_TRY(block_tree_->addBlock(block));

    if (b.justification) {
      OUTCOME_TRY(justification_validator_->validateJustification(
          block_hash, *b.justification));

      OUTCOME_TRY(block_tree_->finalize(block_hash, *b.justification));
    }

    // observe possible changes of authorities
    for (auto &digest_item : block_without_seal_digest.header.digest) {
      OUTCOME_TRY(visit_in_place(
          digest_item,
          [&](const primitives::Consensus &consensus_message)
              -> outcome::result<void> {
            return authority_update_observer_->onConsensus(
                consensus_message.consensus_engine_id,
                primitives::BlockInfo{block.header.number, block_hash},
                consensus_message);
          },
          [](const auto &) { return outcome::success(); }));
    }

    // remove block's extrinsics from tx pool
    for (const auto &extrinsic : block.body) {
      auto res = tx_pool_->removeOne(hasher_->blake2b_256(extrinsic.data));
      if (res.has_error()
          && res
                 != outcome::failure(
                     transaction_pool::TransactionPoolError::TX_NOT_FOUND)) {
        return res;
      }
    }

    auto t_end = std::chrono::high_resolution_clock::now();

    logger_->info(
        "Imported block with number: {}, hash: {} within {} ms",
        block.header.number,
        block_hash.toHex(),
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start)
            .count());
    return outcome::success();
  }

}  // namespace kagome::consensus
