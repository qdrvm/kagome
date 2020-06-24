/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_executor.hpp"

#include "blockchain/block_tree_error.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "scale/scale.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

namespace kagome::consensus {

  BlockExecutor::BlockExecutor(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> core,
      std::shared_ptr<primitives::BabeConfiguration> configuration,
      std::shared_ptr<consensus::BabeSynchronizer> babe_synchronizer,
      std::shared_ptr<consensus::BlockValidator> block_validator,
      std::shared_ptr<consensus::EpochStorage> epoch_storage,
      std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
      std::shared_ptr<crypto::Hasher> hasher)
      : block_tree_{std::move(block_tree)},
        core_{std::move(core)},
        genesis_configuration_{std::move(configuration)},
        babe_synchronizer_{std::move(babe_synchronizer)},
        block_validator_{std::move(block_validator)},
        epoch_storage_{std::move(epoch_storage)},
        tx_pool_{std::move(tx_pool)},
        hasher_{std::move(hasher)},
        logger_{common::createLogger("BlockExecutor")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(core_ != nullptr);
    BOOST_ASSERT(genesis_configuration_ != nullptr);
    BOOST_ASSERT(babe_synchronizer_ != nullptr);
    BOOST_ASSERT(block_validator_ != nullptr);
    BOOST_ASSERT(epoch_storage_ != nullptr);
    BOOST_ASSERT(tx_pool_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
  }

  void BlockExecutor::processNextBlock(
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
        const auto &[last_number, last_hash] = block_tree_->getLastFinalized();
        // we should request blocks between last finalized one and received
        // block
        requestBlocks(
            last_hash, block_hash, babe_header.authority_index, [] {});
      } else {
        requestBlocks(
            header.parent_hash, block_hash, babe_header.authority_index, [] {});
      }
    }
  }

  void BlockExecutor::requestBlocks(const primitives::BlockHeader &new_header,
                                    std::function<void()> &&next) {
    const auto &[last_number, last_hash] = block_tree_->getLastFinalized();
    auto new_block_hash =
        hasher_->blake2b_256(scale::encode(new_header).value());
    BOOST_ASSERT(new_header.number >= last_number);
    auto [_, babe_header] = getBabeDigests(new_header).value();
    return requestBlocks(last_hash,
                         new_block_hash,
                         babe_header.authority_index,
                         std::move(next));
  }

  void BlockExecutor::requestBlocks(const primitives::BlockId &from,
                                    const primitives::BlockHash &to,
                                    primitives::AuthorityIndex authority_index,
                                    std::function<void()> &&next) {
    babe_synchronizer_->request(
        from,
        to,
        authority_index,
        [self_wp{weak_from_this()},
         next(std::move(next))](const std::vector<primitives::Block> &blocks) {
          auto self = self_wp.lock();
          if (not self) return;

          if (blocks.empty()) {
            self->logger_->warn("Received empty list of blocks");
          } else {
            auto front_block_hex =
                self->hasher_
                    ->blake2b_256(scale::encode(blocks.front().header).value())
                    .toHex();
            auto back_block_hex =
                self->hasher_
                    ->blake2b_256(scale::encode(blocks.back().header).value())
                    .toHex();
            self->logger_->info("Received blocks from: {}, to {}",
                                front_block_hex,
                                back_block_hex);
          }
          for (const auto &block : blocks) {
            if (auto apply_res = self->applyBlock(block); not apply_res) {
              if (apply_res
                  == outcome::failure(
                      blockchain::BlockTreeError::BLOCK_EXISTS)) {
                continue;
              }
              self->logger_->warn(
                  "Could not apply block during synchronizing slots.Error: {}",
                  apply_res.error().message());
              break;
            }
          }
          next();
        });
  }

  outcome::result<void> BlockExecutor::applyBlock(
      const primitives::Block &block) {
    auto block_hash = hasher_->blake2b_256(scale::encode(block.header).value());

    // check if block body already exists. If so, do not apply
    if (block_tree_->getBlockBody(block_hash)) {
      return blockchain::BlockTreeError::BLOCK_EXISTS;
    }
    logger_->info(
        "Applying block number: {}, hash: {}",
        block.header.number,
        hasher_->blake2b_256(scale::encode(block.header).value()).toHex());

    OUTCOME_TRY(babe_digests, getBabeDigests(block.header));

    auto [seal, babe_header] = babe_digests;

    auto epoch_index =
        babe_header.slot_number / genesis_configuration_->epoch_length;

    // TODO (kamilsa): PRE-364 uncomment outcome try and remove dirty workaround
    // below
    //    OUTCOME_TRY(this_block_epoch_descriptor,
    //                epoch_storage_->getEpochDescriptor(epoch_index));
    auto this_block_epoch_descriptor_res =
        epoch_storage_->getEpochDescriptor(epoch_index);
    if (not this_block_epoch_descriptor_res) {  // take authorities and
                                                // randomness
                                                // from config
      this_block_epoch_descriptor_res = NextEpochDescriptor{
          .authorities = genesis_configuration_->genesis_authorities,
          .randomness = genesis_configuration_->randomness};
    }
    auto this_block_epoch_descriptor = this_block_epoch_descriptor_res.value();

    auto threshold = calculateThreshold(genesis_configuration_->leadership_rate,
                                        this_block_epoch_descriptor.authorities,
                                        babe_header.authority_index);

    // update authorities and randomnesss
    auto next_epoch_digest_res = getNextEpochDigest(block.header);
    if (next_epoch_digest_res) {
      logger_->info("Got next epoch digest for epoch: {}", epoch_index);
      epoch_storage_
          ->addEpochDescriptor(epoch_index + 2, next_epoch_digest_res.value())
          .value();
    }

    OUTCOME_TRY(block_validator_->validateHeader(
        block.header,
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

    logger_->info("Imported block with number: {}, hash: {}",
                  block.header.number,
                  block_hash.toHex());
    return outcome::success();
  }

}  // namespace kagome::consensus
