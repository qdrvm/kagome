/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/key_value_block_storage.hpp"
#include <utility>
#include "blockchain/impl/persistent_map_util.hpp"
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain,
                            KeyValueBlockStorage::Error,
                            e) {
  using E = kagome::blockchain::KeyValueBlockStorage::Error;
  switch (e) {
    case E::BLOCK_EXISTS:
      return "Block already exists on the chain";
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
      std::shared_ptr<storage::trie::TrieDb> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : storage_{std::move(storage)},
        hasher_{std::move(hasher)},
        logger_{common::createLogger("Block Storage:")} {}

  outcome::result<std::shared_ptr<KeyValueBlockStorage>>
  KeyValueBlockStorage::createWithGenesis(
      const GenesisRawConfig &genesis,
      const std::shared_ptr<storage::trie::TrieDb> &storage,
      std::shared_ptr<crypto::Hasher> hasher) {
    KeyValueBlockStorage kv_storage(storage, std::move(hasher));
    // TODO(Harrm) check that storage is actually empty
    for (const auto &[key, val] : genesis) {
      OUTCOME_TRY(storage->put(key, val));
    }
    // state root type is Hash256, however for consistency with spec root hash
    // returns buffer. So we need this conversion
    OUTCOME_TRY(state_root,
                common::Hash256::fromSpan(storage->getRootHash().toVector()));

    auto extrinsics_root_buf = trieRoot({});
    // same reason for conversion as few lines above
    OUTCOME_TRY(extrinsics_root,
                common::Hash256::fromSpan(extrinsics_root_buf.toVector()));

    // genesis block initialization
    primitives::Block genesis_block;
    genesis_block.header.number = 0;
    genesis_block.header.extrinsics_root = extrinsics_root;
    genesis_block.header.state_root = state_root;
    // the rest of the fields have default value

    OUTCOME_TRY(kv_storage.putBlock(genesis_block));
    return std::make_shared<KeyValueBlockStorage>(kv_storage);
  }

  outcome::result<primitives::BlockHeader> KeyValueBlockStorage::getBlockHeader(
      const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_header, getWithPrefix(*storage_, Prefix::HEADER, id));
    OUTCOME_TRY(header, scale::decode<primitives::BlockHeader>(encoded_header));
    return std::move(header);
  }

  outcome::result<primitives::BlockBody> KeyValueBlockStorage::getBlockBody(
      const primitives::BlockId &id) const {
    OUTCOME_TRY(encoded_body, getWithPrefix(*storage_, Prefix::BODY, id));
    OUTCOME_TRY(body, scale::decode<primitives::BlockBody>(encoded_body));
    return std::move(body);
  }

  outcome::result<primitives::Justification>
  KeyValueBlockStorage::getJustification(
      const primitives::BlockId &block) const {
    OUTCOME_TRY(encoded_just,
                getWithPrefix(*storage_, Prefix::JUSTIFICATION, block));
    OUTCOME_TRY(just, scale::decode<primitives::Justification>(encoded_just));
    return std::move(just);
  }

  outcome::result<primitives::BlockHash> KeyValueBlockStorage::putBlock(
      const primitives::Block &block) {
    OUTCOME_TRY(encoded_block, scale::encode(block));
    auto block_hash = hasher_->blake2b_256(encoded_block);
    auto block_in_storage =
        getWithPrefix(*storage_, Prefix::HEADER, block.header.number);
    if (block_in_storage.has_value()) {
      return Error::BLOCK_EXISTS;
    }
    if (block_in_storage.error() != blockchain::Error::BLOCK_NOT_FOUND) {
      return block_in_storage.error();
    }

    // insert our block's parts into the database-
    OUTCOME_TRY(encoded_header, scale::encode(block.header));
    OUTCOME_TRY(putWithPrefix(*storage_,
                              Prefix::HEADER,
                              block.header.number,
                              block_hash,
                              Buffer{encoded_header}));

    OUTCOME_TRY(encoded_body, scale::encode(block.body));
    OUTCOME_TRY(putWithPrefix(*storage_,
                              Prefix::BODY,
                              block.header.number,
                              block_hash,
                              Buffer{encoded_body}));
    return block_hash;
  }

  outcome::result<void> KeyValueBlockStorage::putJustification(
      const primitives::Justification &j,
      const primitives::BlockHash &hash,
      const primitives::BlockNumber &number) {
    // insert justification into the database
    OUTCOME_TRY(encoded_justification, scale::encode(j));
    OUTCOME_TRY(putWithPrefix(*storage_,
                              Prefix::JUSTIFICATION,
                              number,
                              hash,
                              Buffer{encoded_justification}));
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

    auto body_lookup_key = prependPrefix(block_lookup_key, Prefix::BODY);
    if (auto rm_res = storage_->remove(body_lookup_key); !rm_res) {
      logger_->error("could not remove body from the storage: {}",
                     rm_res.error().message());
      return rm_res;
    }
    return outcome::success();
  }

}  // namespace kagome::blockchain
