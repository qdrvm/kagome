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
  using storage::face::MapCursor;
  using storage::face::WriteBatch;
  using Buffer = common::Buffer;
  using Prefix = prefix::Prefix;

  BlockStorageImpl::BlockStorageImpl(
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : storage_{std::move(storage)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger("BlockStorage", "blockchain")} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
  }

  outcome::result<std::shared_ptr<BlockStorageImpl>> BlockStorageImpl::create(
      storage::trie::RootHash state_root,
      const std::shared_ptr<storage::BufferStorage> &storage,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    auto block_storage = std::shared_ptr<BlockStorageImpl>(
        new BlockStorageImpl(storage, hasher));

    auto res = block_storage->hasBlockHeader(primitives::BlockNumber{0});
    if (res.has_error()) {
      return res.as_failure();
    }

    if (not res.value()) {
      auto extrinsics_root = trieRoot({});

      // genesis block initialization
      primitives::Block genesis_block;
      genesis_block.header.number = 0;
      genesis_block.header.extrinsics_root = extrinsics_root;
      genesis_block.header.state_root = state_root;
      // the rest of the fields have default value

      OUTCOME_TRY(genesis_block_hash, block_storage->putBlock(genesis_block));
      OUTCOME_TRY(block_storage->putNumberToIndexKey({0, genesis_block_hash}));

      OUTCOME_TRY(block_storage->setBlockTreeLeaves({genesis_block_hash}));
    }

    // Fallback way to init block tree leaves list on existed storage
    // TODO(xDimon): After deploy of this change, and using on existing DB,
    //  this code block should be removed
    if (block_storage->getBlockTreeLeaves()
        == outcome::failure(BlockStorageError::BLOCK_TREE_LEAVES_NOT_FOUND)) {
      using namespace common::literals;
      OUTCOME_TRY(last_finalized_block_hash_opt,
                  storage->tryLoad(":kagome:last_finalized_block_hash"_buf));
      if (not last_finalized_block_hash_opt.has_value()) {
        return BlockStorageError::FINALIZED_BLOCK_NOT_FOUND;
      }
      auto &hash = last_finalized_block_hash_opt.value();

      primitives::BlockHash last_finalized_block_hash;
      std::copy(hash.begin(), hash.end(), last_finalized_block_hash.begin());

      OUTCOME_TRY(
          block_storage->setBlockTreeLeaves({last_finalized_block_hash}));
    }

    return block_storage;
  }

  outcome::result<bool> BlockStorageImpl::hasBlockHeader(
      const primitives::BlockId &id) const {
    return hasWithPrefix(*storage_, Prefix::HEADER, id);
  }

  outcome::result<std::optional<primitives::BlockHeader>>
  BlockStorageImpl::getBlockHeader(const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_header_opt,
                getWithPrefix(*storage_, Prefix::HEADER, id));
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
                getWithPrefix(*storage_, Prefix::BLOCK_DATA, id));
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
    OUTCOME_TRY(block_data, getBlockData(block));
    if (block_data.has_value()
        && block_data.value().justification.has_value()) {
      return block_data.value().justification.value();
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
    OUTCOME_TRY(putWithPrefix(*storage_,
                              Prefix::HEADER,
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
    OUTCOME_TRY(putWithPrefix(*storage_,
                              Prefix::BLOCK_DATA,
                              block_number,
                              block_data.hash,
                              Buffer{encoded_block_data}));
    return outcome::success();
  }

  outcome::result<void> BlockStorageImpl::removeBlockData(
      primitives::BlockNumber block_number,
      const primitives::BlockDataFlags &remove_flags) {
    primitives::BlockData to_insert;

    // if block data does not exist, put a new one. Otherwise, get the old one
    // and merge with the new one. During the merge new block data fields have
    // higher priority over the old ones (old ones should be rewritten)
    OUTCOME_TRY(existing_block_data_opt, getBlockData(remove_flags.hash));
    if (not existing_block_data_opt.has_value()) {
      return outcome::success();
    }
    auto &existing_data = existing_block_data_opt.value();

    // add all the fields from the new block_data
    to_insert.header =
        remove_flags.header ? std::nullopt : existing_data.header;
    to_insert.body = remove_flags.body ? std::nullopt : existing_data.body;
    to_insert.justification =
        remove_flags.justification ? std::nullopt : existing_data.justification;
    to_insert.message_queue =
        remove_flags.message_queue ? std::nullopt : existing_data.message_queue;
    to_insert.receipt =
        remove_flags.receipt ? std::nullopt : existing_data.receipt;

    OUTCOME_TRY(encoded_block_data, scale::encode(to_insert));
    OUTCOME_TRY(putWithPrefix(*storage_,
                              Prefix::BLOCK_DATA,
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
    return block_hash;
  }

  outcome::result<void> BlockStorageImpl::putJustification(
      const primitives::Justification &j,
      const primitives::BlockHash &hash,
      primitives::BlockNumber block_number) {
    // insert justification into the database as a part of BlockData
    primitives::BlockData block_data{.hash = hash, .justification = j};
    OUTCOME_TRY(putBlockData(block_number, block_data));
    return outcome::success();
  }

  outcome::result<void> BlockStorageImpl::removeJustification(
      const primitives::BlockHash &hash, primitives::BlockNumber number) {
    primitives::BlockDataFlags flags =
        primitives::BlockDataFlags::allUnset(hash);
    flags.justification = true;
    OUTCOME_TRY(removeBlockData(number, flags));
    return outcome::success();
  }

  outcome::result<void> BlockStorageImpl::removeBlock(
      const primitives::BlockInfo &block) {
    auto block_lookup_key = numberAndHashToLookupKey(block.number, block.hash);

    SL_TRACE(logger_, "Removing block {}...", block);

    auto hash_to_idx_key =
        prependPrefix(Buffer{block.hash}, Prefix::ID_TO_LOOKUP_KEY);
    if (auto res = storage_->remove(hash_to_idx_key); res.has_error()) {
      logger_->error("could not remove hash-to-idx from the storage: {}",
                     res.error().message());
      return res;
    }

    auto num_to_idx_key =
        prependPrefix(numberToIndexKey(block.number), Prefix::ID_TO_LOOKUP_KEY);
    OUTCOME_TRY(num_to_idx_val_opt, storage_->tryLoad(num_to_idx_key.view()));
    if (num_to_idx_val_opt == block_lookup_key) {
      if (auto res = storage_->remove(num_to_idx_key); res.has_error()) {
        SL_ERROR(logger_,
                 "could not remove num-to-idx from the storage: {}",
                 block,
                 res.error().message());
        return res;
      }
      SL_DEBUG(logger_, "Removed num-to-idx of {}", block);
    }

    // TODO(xDimon): needed to clean up trie storage if block deleted
    //  issue: https://github.com/soramitsu/kagome/issues/1128

    auto body_key = prependPrefix(block_lookup_key, Prefix::BLOCK_DATA);
    if (auto res = storage_->remove(body_key); res.has_error()) {
      SL_ERROR(logger_,
               "could not remove body of block {} from the storage: {}",
               block,
               res.error().message());
      return res;
    }

    auto header_key = prependPrefix(block_lookup_key, Prefix::HEADER);
    if (auto res = storage_->remove(header_key); res.has_error()) {
      SL_ERROR(logger_,
               "could not remove header of block {} from the storage: {}",
               block,
               res.error().message());
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

    OUTCOME_TRY(leaves_opt,
                storage_->tryLoad(storage::kBlockTreeLeavesLookupKey));
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

    OUTCOME_TRY(encoded_leaves, scale::encode(leaves));
    OUTCOME_TRY(storage_->put(storage::kBlockTreeLeavesLookupKey,
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
