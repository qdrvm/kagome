/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_storage_impl.hpp"

#include "blockchain/block_storage_error.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "common/visitor.hpp"
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

    OUTCOME_TRY(hash_opt, blockchain::blockHashByNumber(*storage, 0));
    if (not hash_opt.has_value()) {
      // genesis block initialization
      primitives::Block genesis_block;
      genesis_block.header.number = 0;
      genesis_block.header.extrinsics_root = storage::trie::kEmptyRootHash;
      genesis_block.header.state_root = state_root;
      // the rest of the fields have default value

      OUTCOME_TRY(genesis_block_hash, block_storage->putBlock(genesis_block));
      OUTCOME_TRY(block_storage->assignNumberToHash({0, genesis_block_hash}));

      OUTCOME_TRY(block_storage->setBlockTreeLeaves({genesis_block_hash}));

      block_storage->logger_->info(
          "Genesis block {}, state {}", genesis_block_hash, state_root);
    } else {
      auto res = block_storage->hasBlockHeader(hash_opt.value());
      if (res.has_error()) {
        return res.as_failure();
      }
      if (not res.value()) {
        block_storage->logger_->critical(
            "Database is not consistent: Genesis block header not found, "
            "but exists num-to-hash record for block #0");
        return BlockStorageError::HEADER_NOT_FOUND;
      }
    }

    return block_storage;
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

  outcome::result<void> BlockStorageImpl::assignNumberToHash(
      const primitives::BlockInfo &block_info) {
    SL_DEBUG(logger_, "Save num-to-idx for {}", block_info);
    auto num_to_hash_key = blockNumberToKey(block_info.number);
    auto key_space = storage_->getSpace(Space::kLookupKey);
    return key_space->put(num_to_hash_key, block_info.hash);
  }

  outcome::result<void> BlockStorageImpl::deassignNumberToHash(
      primitives::BlockNumber block_number) {
    SL_DEBUG(logger_, "Remove num-to-idx for #{}", block_number);
    auto num_to_hash_key = blockNumberToKey(block_number);
    auto key_space = storage_->getSpace(Space::kLookupKey);
    return key_space->remove(num_to_hash_key);
  }

  outcome::result<std::optional<primitives::BlockHash>>
  BlockStorageImpl::getBlockHash(primitives::BlockNumber block_number) const {
    auto key_space = storage_->getSpace(storage::Space::kLookupKey);
    OUTCOME_TRY(data_opt, key_space->tryGet(blockNumberToKey(block_number)));
    if (data_opt.has_value()) {
      OUTCOME_TRY(hash, primitives::BlockHash::fromSpan(data_opt.value()));
      return hash;
    }
    return std::nullopt;
  }

  outcome::result<std::optional<primitives::BlockHash>>
  BlockStorageImpl::getBlockHash(const primitives::BlockId &block_id) const {
    return visit_in_place(
        block_id,
        [&](const primitives::BlockNumber &block_number)
            -> outcome::result<std::optional<primitives::BlockHash>> {
          auto key_space = storage_->getSpace(storage::Space::kLookupKey);
          OUTCOME_TRY(data_opt,
                      key_space->tryGet(blockNumberToKey(block_number)));
          if (data_opt.has_value()) {
            OUTCOME_TRY(block_hash,
                        primitives::BlockHash::fromSpan(data_opt.value()));
            return block_hash;
          }
          return std::nullopt;
        },
        [](const common::Hash256 &block_hash) { return block_hash; });
  }

  outcome::result<bool> BlockStorageImpl::hasBlockHeader(
      const primitives::BlockHash &block_hash) const {
    return hasInSpace(*storage_, Space::kHeader, block_hash);
  }

  outcome::result<primitives::BlockHash> BlockStorageImpl::putBlockHeader(
      const primitives::BlockHeader &header) {
    OUTCOME_TRY(encoded_header, scale::encode(header));
    auto block_hash = hasher_->blake2b_256(encoded_header);
    OUTCOME_TRY(putToSpace(
        *storage_, Space::kHeader, block_hash, std::move(encoded_header)));
    return block_hash;
  }

  outcome::result<std::optional<primitives::BlockHeader>>
  BlockStorageImpl::getBlockHeader(
      const primitives::BlockHash &block_hash) const {
    OUTCOME_TRY(encoded_header_opt,
                getFromSpace(*storage_, Space::kHeader, block_hash));
    if (encoded_header_opt.has_value()) {
      OUTCOME_TRY(
          header,
          scale::decode<primitives::BlockHeader>(encoded_header_opt.value()));
      return std::move(header);
    }
    return std::nullopt;
  }

  outcome::result<void> BlockStorageImpl::putBlockBody(
      const primitives::BlockHash &block_hash,
      const primitives::BlockBody &block_body) {
    OUTCOME_TRY(encoded_body, scale::encode(block_body));
    return putToSpace(
        *storage_, Space::kBlockBody, block_hash, std::move(encoded_body));
  }

  outcome::result<std::optional<primitives::BlockBody>>
  BlockStorageImpl::getBlockBody(
      const primitives::BlockHash &block_hash) const {
    OUTCOME_TRY(encoded_block_body_opt,
                getFromSpace(*storage_, Space::kBlockBody, block_hash));
    if (encoded_block_body_opt.has_value()) {
      OUTCOME_TRY(
          block_body,
          scale::decode<primitives::BlockBody>(encoded_block_body_opt.value()));
      return std::make_optional(std::move(block_body));
    }
    return std::nullopt;
  }

  outcome::result<void> BlockStorageImpl::removeBlockBody(
      const primitives::BlockHash &block_hash) {
    auto space = storage_->getSpace(Space::kBlockBody);
    return space->remove(block_hash);
  }

  outcome::result<void> BlockStorageImpl::putJustification(
      const primitives::Justification &justification,
      const primitives::BlockHash &hash) {
    BOOST_ASSERT(not justification.data.empty());

    OUTCOME_TRY(encoded_justification, scale::encode(justification));
    OUTCOME_TRY(putToSpace(*storage_,
                           Space::kJustification,
                           hash,
                           std::move(encoded_justification)));

    return outcome::success();
  }

  outcome::result<std::optional<primitives::Justification>>
  BlockStorageImpl::getJustification(
      const primitives::BlockHash &block_hash) const {
    OUTCOME_TRY(encoded_justification_opt,
                getFromSpace(*storage_, Space::kJustification, block_hash));
    if (encoded_justification_opt.has_value()) {
      OUTCOME_TRY(justification,
                  scale::decode<primitives::Justification>(
                      encoded_justification_opt.value()));
      return std::move(justification);
    }
    return std::nullopt;
  }

  outcome::result<void> BlockStorageImpl::removeJustification(
      const primitives::BlockHash &block_hash) {
    auto space = storage_->getSpace(Space::kJustification);
    return space->remove(block_hash);
  }

  outcome::result<primitives::BlockHash> BlockStorageImpl::putBlock(
      const primitives::Block &block) {
    // insert provided block's parts into the database
    OUTCOME_TRY(block_hash, putBlockHeader(block.header));

    primitives::BlockData block_data;
    block_data.hash = block_hash;
    block_data.header = block.header;
    block_data.body = block.body;

    OUTCOME_TRY(encoded_header, scale::encode(block.header));
    OUTCOME_TRY(putToSpace(
        *storage_, Space::kHeader, block_hash, std::move(encoded_header)));

    OUTCOME_TRY(encoded_body, scale::encode(block.body));
    OUTCOME_TRY(putToSpace(
        *storage_, Space::kBlockBody, block_hash, std::move(encoded_body)));

    logger_->info("Added block {} as child of {}",
                  primitives::BlockInfo(block.header.number, block_hash),
                  primitives::BlockInfo(block.header.number - 1,
                                        block.header.parent_hash));
    return std::move(block_hash);
  }

  outcome::result<std::optional<primitives::BlockData>>
  BlockStorageImpl::getBlockData(
      const primitives::BlockHash &block_hash) const {
    primitives::BlockData block_data{.hash = block_hash};

    // Block header
    OUTCOME_TRY(header_opt, getBlockHeader(block_hash));
    block_data.header = std::move(header_opt);

    if (not block_data.header.has_value()) {
      return std::nullopt;
    }

    // Block body
    OUTCOME_TRY(body_opt, getBlockBody(block_hash));
    block_data.body = std::move(body_opt);

    // NOTE: Receipt and MessageQueue should be processed here (not implemented)

    // Justification
    OUTCOME_TRY(justification_opt, getJustification(block_hash));
    block_data.justification = std::move(justification_opt);

    return std::move(block_data);
  }

  outcome::result<void> BlockStorageImpl::removeBlock(
      const primitives::BlockHash &block_hash) {
    // Check if block still in storage
    OUTCOME_TRY(header_opt, getBlockHeader(block_hash));
    if (not header_opt.has_value()) {
      return outcome::success();
    }
    const auto &header = header_opt.value();

    primitives::BlockInfo block_info(header.number, block_hash);

    SL_TRACE(logger_, "Removing block {}...", block_info);

    {  // Remove number-to-key assigning
      auto num_to_hash_key = blockNumberToKey(block_info.number);

      auto key_space = storage_->getSpace(Space::kLookupKey);
      OUTCOME_TRY(hash_opt, key_space->tryGet(num_to_hash_key.view()));
      if (hash_opt == block_hash) {
        if (auto res = key_space->remove(num_to_hash_key); res.has_error()) {
          SL_ERROR(logger_,
                   "could not remove num-to-hash of {} from the storage: {}",
                   block_info,
                   res.error());
          return res;
        }
        SL_DEBUG(logger_, "Removed num-to-idx of {}", block_info);
      }
    }

    // TODO(xDimon): needed to clean up trie storage if block deleted
    //  issue: https://github.com/soramitsu/kagome/issues/1128

    // Remove block body
    if (auto res = removeBlockBody(block_info.hash); res.has_error()) {
      SL_ERROR(logger_,
               "could not remove body of block {} from the storage: {}",
               block_info,
               res.error());
      return res;
    }

    // Remove justification for block
    if (auto res = removeJustification(block_info.hash); res.has_error()) {
      SL_ERROR(
          logger_,
          "could not remove justification for block {} from the storage: {}",
          block_info,
          res.error());
      return res;
    }

    {  // Remove block header
      auto header_space = storage_->getSpace(Space::kHeader);
      if (auto res = header_space->remove(block_info.hash); res.has_error()) {
        SL_ERROR(logger_,
                 "could not remove header of block {} from the storage: {}",
                 block_info,
                 res.error());
        return res;
      }
    }

    logger_->info("Removed block {}", block_info);

    return outcome::success();
  }

}  // namespace kagome::blockchain
