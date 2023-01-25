/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_storage_impl.hpp"

#include "blockchain/block_storage_error.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "scale/scale.hpp"

namespace kagome::blockchain {
  using primitives::Block;
  using primitives::BlockId;
  using storage::Space;
  using Buffer = common::Buffer;

  BlockStorageImpl::BlockStorageImpl(
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : storage_{std::move(storage)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger("BlockStorage", "block_storage")} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
  }

  outcome::result<std::shared_ptr<BlockStorageImpl>> BlockStorageImpl::create(
      storage::trie::RootHash state_root,
      const std::shared_ptr<storage::SpacedStorage> &storage,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    auto block_storage = std::shared_ptr<BlockStorageImpl>(
        new BlockStorageImpl(storage, hasher));

    auto res = block_storage->hasBlockHeader(primitives::BlockNumber{0});
    if (res.has_error()) {
      return res.as_failure();
    }

    if (not res.value()) {
      // genesis block initialization
      primitives::Block genesis_block;
      genesis_block.header.number = 0;
      genesis_block.header.extrinsics_root = storage::trie::kEmptyRootHash;
      genesis_block.header.state_root = state_root;
      // the rest of the fields have default value

      OUTCOME_TRY(genesis_block_hash, block_storage->putBlock(genesis_block));
      OUTCOME_TRY(block_storage->putNumberToIndexKey({0, genesis_block_hash}));

      OUTCOME_TRY(block_storage->setBlockTreeLeaves({genesis_block_hash}));

      block_storage->logger_->info(
          "Genesis block {}, state {}", genesis_block_hash, state_root);
    }

    return block_storage;
  }

  outcome::result<bool> BlockStorageImpl::hasBlockHeader(
      const primitives::BlockId &id) const {
    return hasInSpace(*storage_, Space::kHeader, id);
  }

  outcome::result<std::optional<primitives::BlockHeader>>
  BlockStorageImpl::getBlockHeader(const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_header_opt,
                getFromSpace(*storage_, Space::kHeader, id));
    if (encoded_header_opt.has_value()) {
      OUTCOME_TRY(
          header,
          scale::decode<primitives::BlockHeader>(encoded_header_opt.value()));
      return std::move(header);
    }
    return std::nullopt;
  }

  outcome::result<std::optional<primitives::BlockBody>>
  BlockStorageImpl::getBlockBody(const primitives::BlockId &id) const {
    OUTCOME_TRY(block_data, getBlockData(id));
    if (block_data.has_value() && block_data.value().body.has_value()) {
      return block_data.value().body.value();
    }
    return std::nullopt;
  }

  outcome::result<std::optional<primitives::BlockData>>
  BlockStorageImpl::getBlockData(const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_block_data_opt,
                getFromSpace(*storage_, Space::kBlockData, id));
    if (encoded_block_data_opt.has_value()) {
      OUTCOME_TRY(
          block_data,
          scale::decode<primitives::BlockData>(encoded_block_data_opt.value()));
      return std::move(block_data);
    }
    return std::nullopt;
  }

  outcome::result<std::optional<primitives::Justification>>
  BlockStorageImpl::getJustification(const primitives::BlockId &block) const {
    OUTCOME_TRY(header, getBlockHeader(block));
    BOOST_ASSERT(header.has_value());
    OUTCOME_TRY(encoded_justification_opt,
                getFromSpace(*storage_, Space::kJustification, block));
    if (encoded_justification_opt.has_value()) {
      OUTCOME_TRY(justification,
                  scale::decode<primitives::Justification>(
                      encoded_justification_opt.value()));
      return std::move(justification);
    }
    return std::nullopt;
  }

  outcome::result<void> BlockStorageImpl::putNumberToIndexKey(
      const primitives::BlockInfo &block) {
    SL_DEBUG(logger_, "Save num-to-idx for {}", block);
    return kagome::blockchain::putNumberToIndexKey(*storage_, block);
  }

  outcome::result<primitives::BlockHash> BlockStorageImpl::putBlockHeader(
      const primitives::BlockHeader &header) {
    OUTCOME_TRY(encoded_header, scale::encode(header));
    auto block_hash = hasher_->blake2b_256(encoded_header);
    OUTCOME_TRY(putToSpace(*storage_,
                           Space::kHeader,
                           header.number,
                           block_hash,
                           Buffer{std::move(encoded_header)}));
    return block_hash;
  }

  outcome::result<void> BlockStorageImpl::putBlockData(
      primitives::BlockNumber block_number,
      const primitives::BlockData &block_data) {
    primitives::BlockData to_insert;

    // if block data does not exist, put a new one. Otherwise, get the old one
    // and merge with the new one. During the merge new block data fields have
    // higher priority over the old ones (old ones should be rewritten)
    OUTCOME_TRY(existing_block_data_opt, getBlockData(block_data.hash));
    if (not existing_block_data_opt.has_value()) {
      to_insert = block_data;
    } else {
      auto &existing_data = existing_block_data_opt.value();

      // add all the fields from the new block_data
      to_insert.header =
          block_data.header ? block_data.header : existing_data.header;
      to_insert.body = block_data.body ? block_data.body : existing_data.body;
      to_insert.justification = block_data.justification
                                    ? block_data.justification
                                    : existing_data.justification;
      to_insert.message_queue = block_data.message_queue
                                    ? block_data.message_queue
                                    : existing_data.message_queue;
      to_insert.receipt =
          block_data.receipt ? block_data.receipt : existing_data.receipt;
    }

    OUTCOME_TRY(encoded_block_data, scale::encode(to_insert));
    OUTCOME_TRY(putToSpace(*storage_,
                           Space::kBlockData,
                           block_number,
                           block_data.hash,
                           Buffer{encoded_block_data}));
    return outcome::success();
  }

  outcome::result<void> BlockStorageImpl::removeBlockData(
      primitives::BlockNumber block_number,
      const primitives::BlockDataFlags &remove_flags) {
    primitives::BlockData to_insert;

    OUTCOME_TRY(existing_block_data_opt, getBlockData(remove_flags.hash));
    if (not existing_block_data_opt.has_value()) {
      return outcome::success();
    }
    auto &existing_data = existing_block_data_opt.value();

    auto move_if_flag = [](bool flag, auto &&value) {
      if (flag) {
        return std::move(value);
      }
      return std::optional<typename std::decay_t<decltype(value)>::value_type>(
          std::nullopt);
    };

    // add all the fields from the new block_data
    to_insert.header =
        move_if_flag(!remove_flags.header, std::move(existing_data.header));
    to_insert.body =
        move_if_flag(!remove_flags.body, std::move(existing_data.body));
    to_insert.justification = move_if_flag(
        !remove_flags.justification, std::move(existing_data.justification));
    to_insert.message_queue = move_if_flag(
        !remove_flags.message_queue, std::move(existing_data.message_queue));
    to_insert.receipt =
        move_if_flag(!remove_flags.receipt, std::move(existing_data.receipt));

    OUTCOME_TRY(encoded_block_data, scale::encode(to_insert));
    OUTCOME_TRY(putToSpace(*storage_,
                           Space::kBlockData,
                           block_number,
                           remove_flags.hash,
                           Buffer{encoded_block_data}));
    return outcome::success();
  }

  outcome::result<primitives::BlockHash> BlockStorageImpl::putBlock(
      const primitives::Block &block) {
    // insert our block's parts into the database-
    OUTCOME_TRY(block_hash, putBlockHeader(block.header));

    primitives::BlockData block_data;
    block_data.hash = block_hash;
    block_data.header = block.header;
    block_data.body = block.body;

    OUTCOME_TRY(putBlockData(block.header.number, block_data));
    logger_->info("Added block {} as child of {}",
                  primitives::BlockInfo(block.header.number, block_hash),
                  primitives::BlockInfo(block.header.number - 1,
                                        block.header.parent_hash));
    return std::move(block_hash);
  }

  outcome::result<void> BlockStorageImpl::putJustification(
      const primitives::Justification &j,
      const primitives::BlockHash &hash,
      primitives::BlockNumber block_number) {
    BOOST_ASSERT(not j.data.empty());

    OUTCOME_TRY(justification, scale::encode(j));
    OUTCOME_TRY(putToSpace(*storage_,
                           Space::kJustification,
                           block_number,
                           hash,
                           Buffer{justification}));

    // the following is still required
    primitives::BlockData block_data{.hash = hash};
    OUTCOME_TRY(putBlockData(block_number, block_data));
    return outcome::success();
  }

  outcome::result<void> BlockStorageImpl::removeJustification(
      const primitives::BlockHash &hash, primitives::BlockNumber number) {
    auto key = numberAndHashToLookupKey(number, hash);
    auto space = storage_->getSpace(Space::kJustification);

    OUTCOME_TRY(space->remove(key));
    return outcome::success();
  }

  outcome::result<void> BlockStorageImpl::removeBlock(
      const primitives::BlockInfo &block) {
    auto block_lookup_key = numberAndHashToLookupKey(block.number, block.hash);

    SL_TRACE(logger_, "Removing block {}...", block);
    auto key_space = storage_->getSpace(Space::kLookupKey);

    if (auto res = key_space->remove(block.hash); res.has_error()) {
      logger_->error("could not remove hash-to-idx from the storage: {}",
                     res.error());
      return res;
    }

    auto num_to_idx_key = numberToIndexKey(block.number);
    OUTCOME_TRY(num_to_idx_val_opt, key_space->tryGet(num_to_idx_key.view()));
    if (num_to_idx_val_opt == block_lookup_key) {
      if (auto res = key_space->remove(num_to_idx_key); res.has_error()) {
        SL_ERROR(logger_,
                 "could not remove num-to-idx from the storage: {}",
                 block,
                 res.error());
        return res;
      }
      SL_DEBUG(logger_, "Removed num-to-idx of {}", block);
    }

    // TODO(xDimon): needed to clean up trie storage if block deleted
    //  issue: https://github.com/soramitsu/kagome/issues/1128

    auto block_data_space = storage_->getSpace(Space::kBlockData);
    if (auto res = block_data_space->remove(block_lookup_key);
        res.has_error()) {
      SL_ERROR(logger_,
               "could not remove body of block {} from the storage: {}",
               block,
               res.error());
      return res;
    }

    auto header_space = storage_->getSpace(Space::kHeader);
    if (auto res = header_space->remove(block_lookup_key); res.has_error()) {
      SL_ERROR(logger_,
               "could not remove header of block {} from the storage: {}",
               block,
               res.error());
      return res;
    }

    if (auto res = removeJustification(block.hash, block.number);
        res.has_error()) {
      SL_ERROR(
          logger_,
          "could not remove justification for block {} from the storage: {}",
          block,
          res.error());
      return res;
    }

    logger_->info("Removed block {}", block);

    return outcome::success();
  }

  outcome::result<std::vector<primitives::BlockHash>>
  BlockStorageImpl::getBlockTreeLeaves() const {
    if (block_tree_leaves_.has_value()) {
      return block_tree_leaves_.value();
    }

    auto default_space = storage_->getSpace(Space::kDefault);
    OUTCOME_TRY(leaves_opt,
                default_space->tryGet(storage::kBlockTreeLeavesLookupKey));
    if (not leaves_opt.has_value()) {
      return BlockStorageError::BLOCK_TREE_LEAVES_NOT_FOUND;
    }

    OUTCOME_TRY(
        leaves,
        scale::decode<std::vector<primitives::BlockHash>>(leaves_opt.value()));

    block_tree_leaves_.emplace(std::move(leaves));

    return block_tree_leaves_.value();
  }

  outcome::result<void> BlockStorageImpl::setBlockTreeLeaves(
      std::vector<primitives::BlockHash> leaves) {
    if (block_tree_leaves_.has_value()
        and block_tree_leaves_.value() == leaves) {
      return outcome::success();
    }

    auto default_space = storage_->getSpace(Space::kDefault);
    OUTCOME_TRY(encoded_leaves, scale::encode(leaves));
    OUTCOME_TRY(default_space->put(storage::kBlockTreeLeavesLookupKey,
                                   Buffer{std::move(encoded_leaves)}));

    block_tree_leaves_.emplace(std::move(leaves));

    return outcome::success();
  }

  outcome::result<primitives::BlockInfo> BlockStorageImpl::getLastFinalized()
      const {
    OUTCOME_TRY(leaves, getBlockTreeLeaves());
    auto current_hash = leaves[0];
    for (;;) {
      OUTCOME_TRY(j_opt, getJustification(current_hash));
      if (j_opt.has_value()) {
        break;
      }
      OUTCOME_TRY(header_opt, getBlockHeader(current_hash));
      if (header_opt.has_value()) {
        auto header = header_opt.value();
        if (header.number == 0) {
          SL_TRACE(logger_,
                   "Not found block with justification. "
                   "Genesis block will be used as last finalized ({})",
                   current_hash);
          return {0, current_hash};  // genesis
        }
        current_hash = header.parent_hash;
      } else {
        SL_ERROR(
            logger_, "Failed to fetch header for block ({})", current_hash);
        return BlockStorageError::HEADER_NOT_FOUND;
      }
    }

    OUTCOME_TRY(header, getBlockHeader(current_hash));
    primitives::BlockInfo found_block{header.value().number, current_hash};
    SL_TRACE(logger_,
             "Justification is found in block {}. "
             "This block will be used as last finalized",
             found_block);
    return found_block;
  }

}  // namespace kagome::blockchain
