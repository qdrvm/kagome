/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_executor.hpp"

#include "blockchain/block_tree_error.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus {

  BlockExecutor::BlockExecutor(
      std::shared_ptr<kagome::blockchain::BlockTree> block_tree,
      std::shared_ptr<kagome::runtime::Core> core,
      std::shared_ptr<primitives::BabeConfiguration> configuration,
      std::shared_ptr<kagome::consensus::BabeSynchronizer> babe_synchronizer,
      std::shared_ptr<kagome::consensus::BlockValidator> block_validator,
      std::shared_ptr<kagome::consensus::EpochStorage> epoch_storage,
      std::shared_ptr<kagome::crypto::Hasher> hasher)
      : block_tree_{std::move(block_tree)},
        core_{std::move(core)},
        genesis_configuration_{std::move(configuration)},
        babe_synchronizer_{std::move(babe_synchronizer)},
        block_validator_{std::move(block_validator)},
        epoch_storage_{std::move(epoch_storage)},
        hasher_{std::move(hasher)},
        logger_{common::createLogger("BlockExecutor")} {}

  void BlockExecutor::processNextBlock(
      const primitives::BlockHeader &header,
      const std::function<void(const primitives::BlockHeader &)>
          &new_block_handler) {
    auto block_hash = hasher_->blake2b_256(scale::encode(header).value());

    // insert block_header if it is missing
    if (not block_tree_->getBlockHeader(block_hash)) {
      if (auto add_res = block_tree_->addBlockHeader(header); not add_res) {
        logger_->warn(
            "Could not add block header during import. Number: {}, Hash: {}, "
            "Reason: {}",
            header.number,
            block_hash.toHex(),
            add_res.error().message());
      }
      // handle if block exists
      else {
        new_block_handler(header);
        logger_->info("Added block header. Number: {}, Hash: {}",
                      header.number,
                      block_hash.toHex());
      }
      const auto &[best_number, best_hash] =
          block_tree_->getLastFinalized();  // ?? or getDeepestLeaf

      // we should request block
      synchronizeBlocks(best_hash, block_hash, [] {});
    }
  }

  void BlockExecutor::synchronizeBlocks(
      const primitives::BlockHeader &new_header, std::function<void()> next) {
    const auto &[last_number, last_hash] = block_tree_->getLastFinalized();
    auto new_block_hash =
        hasher_->blake2b_256(scale::encode(new_header).value());
    BOOST_ASSERT(new_header.number >= last_number);
    return synchronizeBlocks(last_hash, new_block_hash, std::move(next));
  }

  void BlockExecutor::synchronizeBlocks(const primitives::BlockId &from,
                                        const primitives::BlockHash &to,
                                        std::function<void()> next) {
    babe_synchronizer_->request(
        from,
        to,
        [self{shared_from_this()},
         next(std::move(next))](const std::vector<primitives::Block> &blocks) {
          if (blocks.empty()) {
            self->logger_->error("Received empty list of blocks");
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
              self->logger_->error(
                  "Could not apply block during synchronizing slots.Error: {}",
                  apply_res.error().message());
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

    auto threshold = calculateThreshold(genesis_configuration_->c,
                                        this_block_epoch_descriptor.authorities,
                                        babe_header.authority_index);

    // update authorities and randomnesss
    auto next_epoch_digest_res = getNextEpochDigest(block.header);
    if (next_epoch_digest_res) {
      logger_->info("Got next epoch digest for epoch: {}", epoch_index);
      epoch_storage_->addEpochDescriptor(epoch_index,
                                         next_epoch_digest_res.value());
    }

    OUTCOME_TRY(block_validator_->validateHeader(
        block.header,
        this_block_epoch_descriptor.authorities[babe_header.authority_index].id,
        threshold,
        this_block_epoch_descriptor.randomness));

    auto block_without_digest = block;

    // block should be applied without last digest which contains the seal
    block_without_digest.header.digest.pop_back();
    // apply block
    OUTCOME_TRY(core_->execute_block(block_without_digest));

    // add block header if it does not exist
    if (not block_tree_->getBlockHeader(block_hash).has_value()) {
      OUTCOME_TRY(block_tree_->addBlockHeader(block.header));
    }
    OUTCOME_TRY(
        block_tree_->addBlockBody(block.header.number, block_hash, block.body));

    logger_->info("Imported block with number: {}, hash: {}",
                  block.header.number,
                  block_hash.toHex());
    return outcome::success();
  }

}  // namespace kagome::consensus
