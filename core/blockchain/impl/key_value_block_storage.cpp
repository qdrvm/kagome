/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/key_value_block_storage.hpp"

#include "blockchain/impl/storage_util.hpp"
#include "scale/scale.hpp"

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
        logger_{common::createLogger("Block Storage:")} {}

  outcome::result<std::shared_ptr<KeyValueBlockStorage>>
  KeyValueBlockStorage::createWithGenesis(
      common::Buffer state_root,
      const std::shared_ptr<storage::BufferStorage> &storage,
      std::shared_ptr<crypto::Hasher> hasher,
      const GenesisHandler &on_genesis_created) {
    KeyValueBlockStorage block_storage(storage, std::move(hasher));
    // TODO(Harrm) check that storage is actually empty

    // state root type is Hash256, however for consistency with spec root hash
    // returns buffer. So we need this conversion
    OUTCOME_TRY(state_root_blob,
                common::Hash256::fromSpan(state_root.toVector()));

    auto extrinsics_root_buf = trieRoot({});
    // same reason for conversion as few lines above
    OUTCOME_TRY(extrinsics_root,
                common::Hash256::fromSpan(extrinsics_root_buf.toVector()));

    // genesis block initialization
    primitives::Block genesis_block;
    genesis_block.header.number = 0;
    genesis_block.header.extrinsics_root = extrinsics_root;
    genesis_block.header.state_root = state_root_blob;
    // the rest of the fields have default value

    OUTCOME_TRY(block_storage.putBlock(genesis_block));
    on_genesis_created(genesis_block);
    return std::make_shared<KeyValueBlockStorage>(block_storage);
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
                              Buffer{encoded_header}));
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
    auto block_in_storage =
        getWithPrefix(*storage_, Prefix::HEADER, block.header.number);
    if (block_in_storage.has_value()) {
      return Error::BLOCK_EXISTS;
    }
    if (block_in_storage.error() != blockchain::Error::BLOCK_NOT_FOUND) {
      return block_in_storage.error();
    }

    // insert our block's parts into the database-
    OUTCOME_TRY(block_hash, putBlockHeader(block.header));

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

}  // namespace kagome::blockchain
