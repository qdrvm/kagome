/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/key_value_block_storage.hpp"

#include "blockchain/impl/storage_util.hpp"
#include "scale/scale.hpp"
#include "storage/database_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain,
                            KeyValueBlockStorage::Error,
                            e) {
  using E = kagome::blockchain::KeyValueBlockStorage::Error;
  switch (e) {
    case E::BLOCK_EXISTS:
      return "Block already exists on the chain";
    case E::BODY_DOES_NOT_EXIST:
      return "Block body was not found";
    case E::JUSTIFICATION_DOES_NOT_EXIST:
      return "Justification was not found";
    case E::GENESIS_BLOCK_ALREADY_EXISTS:
      return "Genesis block already exists";
    case E::FINALIZED_BLOCK_NOT_FOUND:
      return "Finalized block not found. Possible storage corrupted";
    case E::GENESIS_BLOCK_NOT_FOUND:
      return "Genesis block not found exists";
  }
  return "Unknown error";
}

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
      const std::shared_ptr<crypto::Hasher> &hasher,
      const BlockHandler &on_finalized_block_found) {
    auto block_storage = std::make_shared<KeyValueBlockStorage>(
        KeyValueBlockStorage(storage, hasher));

    auto last_finalized_block_hash_res =
        block_storage->getLastFinalizedBlockHash();

    if (last_finalized_block_hash_res.has_value()) {
      return loadExisting(storage, hasher, on_finalized_block_found);
    }

    if (last_finalized_block_hash_res
        == outcome::failure(Error::FINALIZED_BLOCK_NOT_FOUND)) {
      return createWithGenesis(
          std::move(state_root), storage, hasher, on_finalized_block_found);
    }

    return last_finalized_block_hash_res.error();
  }

  outcome::result<std::shared_ptr<KeyValueBlockStorage>>
  KeyValueBlockStorage::loadExisting(
      const std::shared_ptr<storage::BufferStorage> &storage,
      std::shared_ptr<crypto::Hasher> hasher,
      const BlockHandler &on_finalized_block_found) {
    auto block_storage = std::make_shared<KeyValueBlockStorage>(
        KeyValueBlockStorage(storage, std::move(hasher)));

    OUTCOME_TRY(last_finalized_block_hash,
                block_storage->getLastFinalizedBlockHash());

    OUTCOME_TRY(block_header,
                block_storage->getBlockHeader(last_finalized_block_hash));

    primitives::Block finalized_block;
    finalized_block.header = block_header;

    on_finalized_block_found(finalized_block);

    return block_storage;
  }

  outcome::result<std::shared_ptr<KeyValueBlockStorage>>
  KeyValueBlockStorage::createWithGenesis(
      storage::trie::RootHash state_root,
      const std::shared_ptr<storage::BufferStorage> &storage,
      std::shared_ptr<crypto::Hasher> hasher,
      const BlockHandler &on_genesis_created) {
    auto block_storage = std::make_shared<KeyValueBlockStorage>(
        KeyValueBlockStorage(storage, std::move(hasher)));

    OUTCOME_TRY(block_storage->ensureGenesisNotExists());

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
    OUTCOME_TRY(block_storage->setLastFinalizedBlockHash(genesis_block_hash));

    on_genesis_created(genesis_block);
    return block_storage;
  }

  outcome::result<primitives::BlockHeader> KeyValueBlockStorage::getBlockHeader(
      const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_header, getWithPrefix(*storage_, Prefix::HEADER, id));
    OUTCOME_TRY(header, scale::decode<primitives::BlockHeader>(encoded_header));
    return std::move(header);
  }

  outcome::result<primitives::BlockBody> KeyValueBlockStorage::getBlockBody(
      const primitives::BlockId &id) const {
    OUTCOME_TRY(block_data, getBlockData(id));
    if (block_data.body) {
      return block_data.body.value();
    }
    return Error::BODY_DOES_NOT_EXIST;
  }

  outcome::result<primitives::BlockData> KeyValueBlockStorage::getBlockData(
      const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_block_data,
                getWithPrefix(*storage_, Prefix::BLOCK_DATA, id));
    OUTCOME_TRY(block_data,
                scale::decode<primitives::BlockData>(encoded_block_data));
    return std::move(block_data);
  }

  outcome::result<primitives::Justification>
  KeyValueBlockStorage::getJustification(
      const primitives::BlockId &block) const {
    OUTCOME_TRY(block_data, getBlockData(block));
    if (block_data.justification) {
      return block_data.justification.value();
    }
    return Error::JUSTIFICATION_DOES_NOT_EXIST;
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
      return Error::BLOCK_EXISTS;
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
    logger_->info("Added block. Number: {}. Hash: {}. State root: {}",
                  block.header.number,
                  block_hash.toHex(),
                  block.header.state_root.toHex());
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
    auto hash_res = storage_->get(storage::kGenesisBlockHashLookupKey);
    if (hash_res.has_value()) {
      primitives::BlockHash hash;
      std::copy(hash_res.value().begin(), hash_res.value().end(), hash.begin());
      return hash;
    }

    if (hash_res == outcome::failure(storage::DatabaseError::NOT_FOUND)) {
      return Error::GENESIS_BLOCK_NOT_FOUND;
    }

    return hash_res.as_failure();
  }

  outcome::result<primitives::BlockHash>
  KeyValueBlockStorage::getLastFinalizedBlockHash() const {
    auto hash_res = storage_->get(storage::kLastFinalizedBlockHashLookupKey);
    if (hash_res.has_value()) {
      primitives::BlockHash hash;
      std::copy(hash_res.value().begin(), hash_res.value().end(), hash.begin());
      return hash;
    }

    if (hash_res == outcome::failure(storage::DatabaseError::NOT_FOUND)) {
      return Error::FINALIZED_BLOCK_NOT_FOUND;
    }

    return hash_res.as_failure();
  }

  outcome::result<void> KeyValueBlockStorage::setLastFinalizedBlockHash(
      const primitives::BlockHash &hash) {
    OUTCOME_TRY(
        storage_->put(storage::kLastFinalizedBlockHashLookupKey, Buffer{hash}));

    return outcome::success();
  }

  outcome::result<void> KeyValueBlockStorage::ensureGenesisNotExists() const {
    auto res = getLastFinalizedBlockHash();
    if (res.has_value()) {
      return Error::GENESIS_BLOCK_ALREADY_EXISTS;
    }
    return outcome::success();
  }
}  // namespace kagome::blockchain
