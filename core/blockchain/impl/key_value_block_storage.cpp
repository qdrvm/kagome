/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/key_value_block_storage.hpp"

#include "blockchain/block_storage_error.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "scale/scale.hpp"
#include "storage/database_error.hpp"

namespace kagome::blockchain {
  using primitives::Block;
  using primitives::BlockId;
  using storage::face::MapCursor;
  using storage::face::WriteBatch;
  using Buffer = common::Buffer;
  using Prefix = prefix::Prefix;

  KeyValueBlockStorage::KeyValueBlockStorage(
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : storage_{std::move(storage)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger("BlockStorage", "blockchain")} {}

  outcome::result<std::shared_ptr<KeyValueBlockStorage>>
  KeyValueBlockStorage::create(
      storage::trie::RootHash state_root,
      const std::shared_ptr<storage::BufferStorage> &storage,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    auto block_storage = std::make_shared<KeyValueBlockStorage>(
        KeyValueBlockStorage(storage, hasher));

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

      OUTCOME_TRY(storage->put(storage::kGenesisBlockHashLookupKey,
                               Buffer{genesis_block_hash}));

      OUTCOME_TRY(block_storage->setBlockTreeLeaves({genesis_block_hash}));

      OUTCOME_TRY(block_storage->setLastFinalizedBlockHash(genesis_block_hash));
    }

    return std::move(block_storage);
  }

  outcome::result<bool> KeyValueBlockStorage::hasBlockHeader(
      const primitives::BlockId &id) const {
    return hasWithPrefix(*storage_, Prefix::HEADER, id);
  }

  outcome::result<primitives::BlockHeader> KeyValueBlockStorage::getBlockHeader(
      const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_header_opt,
                getWithPrefix(*storage_, Prefix::HEADER, id));
    if (encoded_header_opt.has_value()) {
      OUTCOME_TRY(
          header,
          scale::decode<primitives::BlockHeader>(encoded_header_opt.value()));
      return std::move(header);
    }
    return BlockStorageError::HEADER_DOES_NOT_EXIST;
  }

  outcome::result<primitives::BlockBody> KeyValueBlockStorage::getBlockBody(
      const primitives::BlockId &id) const {
    OUTCOME_TRY(block_data, getBlockData(id));
    if (block_data.body) {
      return block_data.body.value();
    }
    return BlockStorageError::BODY_DOES_NOT_EXIST;
  }

  outcome::result<primitives::BlockData> KeyValueBlockStorage::getBlockData(
      const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_block_data_opt,
                getWithPrefix(*storage_, Prefix::BLOCK_DATA, id));
    if (encoded_block_data_opt.has_value()) {
      OUTCOME_TRY(
          block_data,
          scale::decode<primitives::BlockData>(encoded_block_data_opt.value()));
      return std::move(block_data);
    }
    return BlockStorageError::BLOCK_DATA_DOES_NOT_EXIST;
  }

  outcome::result<primitives::Justification>
  KeyValueBlockStorage::getJustification(
      const primitives::BlockId &block) const {
    OUTCOME_TRY(block_data, getBlockData(block));
    if (block_data.justification) {
      return block_data.justification.value();
    }
    return BlockStorageError::JUSTIFICATION_DOES_NOT_EXIST;
  }

  outcome::result<primitives::BlockHash> KeyValueBlockStorage::putBlockHeader(
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

  outcome::result<void> KeyValueBlockStorage::putBlockData(
      primitives::BlockNumber block_number,
      const primitives::BlockData &block_data) {
    primitives::BlockData to_insert;

    // if block data does not exist, put a new one. Otherwise get the old one
    // and merge with the new one. During the merge new block data fields have
    // higher priority over the old ones (old ones should be rewritten)
    auto existing_block_data_res = getBlockData(block_data.hash);
    if (not existing_block_data_res) {
      to_insert = block_data;
    } else {
      auto existing_data = existing_block_data_res.value();

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

  outcome::result<primitives::BlockHash> KeyValueBlockStorage::putBlock(
      const primitives::Block &block) {
    // TODO(xDimon): Need to implement mechanism for wiping out orphan blocks
    //  (in side-chains rejected by finalization)
    //  to avoid storage space leaks
    auto block_hash = hasher_->blake2b_256(scale::encode(block.header).value());
    auto block_in_storage_res =
        getWithPrefix(*storage_, Prefix::HEADER, block_hash);
    if (block_in_storage_res.has_value()) {
      return BlockStorageError::BLOCK_EXISTS;
    }
    if (block_in_storage_res
        != outcome::failure(blockchain::Error::BLOCK_NOT_FOUND)) {
      return block_in_storage_res.error();
    }

    // insert our block's parts into the database-
    OUTCOME_TRY(putBlockHeader(block.header));

    primitives::BlockData block_data;
    block_data.hash = block_hash;
    block_data.header = block.header;
    block_data.body = block.body;

    OUTCOME_TRY(putBlockData(block.header.number, block_data));
    logger_->info("Added block {}. State root: {}",
                  primitives::BlockInfo(block.header.number, block_hash),
                  block.header.state_root);
    return block_hash;
  }

  outcome::result<void> KeyValueBlockStorage::putJustification(
      const primitives::Justification &j,
      const primitives::BlockHash &hash,
      const primitives::BlockNumber &block_number) {
    // insert justification into the database as a part of BlockData
    primitives::BlockData block_data{.hash = hash, .justification = j};
    OUTCOME_TRY(putBlockData(block_number, block_data));
    return outcome::success();
  }

  outcome::result<void> KeyValueBlockStorage::removeBlock(
      const primitives::BlockHash &hash,
      const primitives::BlockNumber &number) {
    auto block_lookup_key = numberAndHashToLookupKey(number, hash);
    auto header_lookup_key = prependPrefix(block_lookup_key, Prefix::HEADER);
    if (auto rm_res = storage_->remove(header_lookup_key); !rm_res) {
      logger_->error("could not remove header from the storage: {}",
                     rm_res.error().message());
      return rm_res;
    }

    auto body_lookup_key = prependPrefix(block_lookup_key, Prefix::BLOCK_DATA);
    if (auto rm_res = storage_->remove(body_lookup_key); !rm_res) {
      logger_->error("could not remove body from the storage: {}",
                     rm_res.error().message());
      return rm_res;
    }
    return outcome::success();
  }

  outcome::result<primitives::BlockHash>
  KeyValueBlockStorage::getGenesisBlockHash() const {
    if (genesis_block_hash_.has_value()) {
      return genesis_block_hash_.value();
    }

    auto hash_res = storage_->get(storage::kGenesisBlockHashLookupKey);
    if (hash_res.has_value()) {
      primitives::BlockHash hash;
      std::copy(hash_res.value().begin(), hash_res.value().end(), hash.begin());
      const_cast<decltype(genesis_block_hash_) &>(genesis_block_hash_) = hash;
      return hash;
    }

    if (hash_res == outcome::failure(storage::DatabaseError::NOT_FOUND)) {
      return BlockStorageError::GENESIS_BLOCK_NOT_FOUND;
    }

    return hash_res.as_failure();
  }

  outcome::result<std::vector<primitives::BlockHash>>
  KeyValueBlockStorage::getBlockTreeLeaves() const {
    if (block_tree_leaves_.has_value()) {
      return block_tree_leaves_.value();
    }

    OUTCOME_TRY(leaves_opt,
                storage_->tryGet(storage::kBlockTreeLeavesLookupKey));
    if (not leaves_opt.has_value()) {
      return BlockStorageError::BLOCK_TREE_LEAVES_NOT_FOUND;
    }

    OUTCOME_TRY(
        leaves,
        scale::decode<std::vector<primitives::BlockHash>>(leaves_opt.value()));

    block_tree_leaves_.emplace(std::move(leaves));

    return block_tree_leaves_.value();
  }

  outcome::result<void> KeyValueBlockStorage::setBlockTreeLeaves(
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

  outcome::result<primitives::BlockHash>
  KeyValueBlockStorage::getLastFinalizedBlockHash() const {
    if (last_finalized_block_hash_.has_value()) {
      return last_finalized_block_hash_.value();
    }

    OUTCOME_TRY(hash_opt,
                storage_->tryGet(storage::kLastFinalizedBlockHashLookupKey));
    if (not hash_opt.has_value()) {
      return BlockStorageError::FINALIZED_BLOCK_NOT_FOUND;
    }

    primitives::BlockHash hash;
    std::copy(hash_opt.value().begin(), hash_opt.value().end(), hash.begin());

    last_finalized_block_hash_.emplace(hash);

    return hash;
  }

  outcome::result<void> KeyValueBlockStorage::setLastFinalizedBlockHash(
      const primitives::BlockHash &hash) {
    OUTCOME_TRY(
        storage_->put(storage::kLastFinalizedBlockHashLookupKey, Buffer{hash}));
    last_finalized_block_hash_.emplace(hash);
    return outcome::success();
  }
}  // namespace kagome::blockchain
