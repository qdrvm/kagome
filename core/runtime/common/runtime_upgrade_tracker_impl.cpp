/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_upgrade_tracker_impl.hpp"

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/common/storage_code_provider.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::runtime {
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s,
                     const RuntimeUpgradeTrackerImpl::RuntimeUpgradeData &d) {
    return s << d.block << d.state;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s,
                     RuntimeUpgradeTrackerImpl::RuntimeUpgradeData &d) {
    return s >> d.block >> d.state;
  }

  outcome::result<std::unique_ptr<RuntimeUpgradeTrackerImpl>>
  RuntimeUpgradeTrackerImpl::create(
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<const primitives::CodeSubstituteBlockIds>
          code_substitutes) {
    BOOST_ASSERT(header_repo);
    BOOST_ASSERT(storage);
    BOOST_ASSERT(code_substitutes);

    OUTCOME_TRY(encoded_opt, storage->tryLoad(storage::kRuntimeHashesLookupKey));

    std::vector<RuntimeUpgradeData> saved_data{};
    if (encoded_opt.has_value()) {
      OUTCOME_TRY(
          decoded,
          scale::decode<std::vector<RuntimeUpgradeData>>(encoded_opt.value()));
      saved_data = std::move(decoded);
    }
    return std::unique_ptr<RuntimeUpgradeTrackerImpl>{
        new RuntimeUpgradeTrackerImpl(std::move(header_repo),
                                      std::move(storage),
                                      std::move(code_substitutes),
                                      std::move(saved_data))};
  }

  RuntimeUpgradeTrackerImpl::RuntimeUpgradeTrackerImpl(
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<const primitives::CodeSubstituteBlockIds>
          code_substitutes,
      std::vector<RuntimeUpgradeData> &&saved_data)
      : runtime_upgrades_{std::move(saved_data)},
        header_repo_{std::move(header_repo)},
        storage_{std::move(storage)},
        known_code_substitutes_{std::move(code_substitutes)},
        logger_{log::createLogger("StorageCodeProvider", "runtime")} {}

  bool RuntimeUpgradeTrackerImpl::hasCodeSubstitute(
      const kagome::primitives::BlockInfo &block_info) const {
    return known_code_substitutes_->contains(block_info);
  }

  bool RuntimeUpgradeTrackerImpl::isStateInChain(
      const primitives::BlockInfo &state,
      const primitives::BlockInfo &chain_end) const {
    // if the found state is finalized, it is guaranteed to not belong to a
    // different fork
    if (block_tree_->getLastFinalized().number >= state.number) {
      return true;
    }
    // a non-finalized state may belong to a different fork, need to check
    // explicitly (can be expensive if blocks are far apart)
    KAGOME_PROFILE_START(has_direct_chain)
    bool has_direct_chain =
        block_tree_->hasDirectChain(state.hash, chain_end.hash);
    KAGOME_PROFILE_END(has_direct_chain)
    return has_direct_chain;
  }

  outcome::result<std::optional<storage::trie::RootHash>>
  RuntimeUpgradeTrackerImpl::findProperFork(
      const primitives::BlockInfo &block,
      std::vector<RuntimeUpgradeData>::const_reverse_iterator latest_upgrade_it)
      const {
    for (; latest_upgrade_it != runtime_upgrades_.rend(); latest_upgrade_it++) {
      if (isStateInChain(latest_upgrade_it->block, block)) {
        SL_TRACE_FUNC_CALL(
            logger_, latest_upgrade_it->state, block.hash, block.number);
        SL_DEBUG(logger_,
                 "Pick runtime state at block {} for block {}",
                 latest_upgrade_it->block,
                 block);

        return latest_upgrade_it->state;
      }
    }
    return std::nullopt;
  }

  outcome::result<storage::trie::RootHash>
  RuntimeUpgradeTrackerImpl::getLastCodeUpdateState(
      const primitives::BlockInfo &block) {
    // if the block tree is not yet initialized, means we can only access the
    // genesis block
    if (block_tree_ == nullptr) {
      OUTCOME_TRY(genesis, header_repo_->getBlockHeader(0));
      SL_DEBUG(logger_, "Pick runtime state at genesis for block {}", block);
      return genesis.state_root;
    }

    if (hasCodeSubstitute(block)) {
      OUTCOME_TRY(push(block.hash));
    }

    // if there are no known blocks with runtime upgrades, we just fall back to
    // returning the state of the current block
    if (runtime_upgrades_.empty()) {
      // even if it doesn't actually upgrade runtime, still a solid source of
      // runtime code
      OUTCOME_TRY(state, push(block.hash));
      SL_DEBUG(
          logger_, "Pick runtime state at block {} for the same block", block);
      return std::move(state);
    }

    KAGOME_PROFILE_START(blocks_with_runtime_upgrade_search)
    auto block_number = block.number;
    auto latest_upgrade =
        std::upper_bound(runtime_upgrades_.begin(),
                         runtime_upgrades_.end(),
                         block_number,
                         [](auto block_number, auto const &upgrade_data) {
                           return block_number < upgrade_data.block.number;
                         });
    KAGOME_PROFILE_END(blocks_with_runtime_upgrade_search)

    if (latest_upgrade == runtime_upgrades_.begin()) {
      // if we have no info on updates before this block, we just return its
      // state
      OUTCOME_TRY(block_header, header_repo_->getBlockHeader(block.hash));
      SL_DEBUG(
          logger_, "Pick runtime state at block {} for the same block", block);
      return block_header.state_root;
    }

    // this conversion also steps back on one element
    auto reverse_latest_upgrade = std::make_reverse_iterator(latest_upgrade);

    // we are now at the last element in block_with_runtime_upgrade which is
    // less or equal to our \arg block number
    // we may have several entries with the same block number, we have to pick
    // one which is the predecessor of our block
    KAGOME_PROFILE_START(search_for_proper_fork)
    OUTCOME_TRY(proper_fork, findProperFork(block, reverse_latest_upgrade));
    KAGOME_PROFILE_END(search_for_proper_fork)
    if (proper_fork.has_value()) {
      return proper_fork.value();
    }
    // if this is an orphan block for some reason, just return its state_root
    // (there is no other choice)
    OUTCOME_TRY(block_header, header_repo_->getBlockHeader(block.hash));
    logger_->warn("Block {}, a child of block {} is orphan",
                  block,
                  primitives::BlockInfo(block_header.number - 1,
                                        block_header.parent_hash));
    return block_header.state_root;
  }

  outcome::result<primitives::BlockInfo>
  RuntimeUpgradeTrackerImpl::getLastCodeUpdateBlockInfo(
      const storage::trie::RootHash &state) const {
    auto it = std::find_if(
        runtime_upgrades_.begin(),
        runtime_upgrades_.end(),
        [&state](const auto &item) { return state == item.state; });
    if (it != runtime_upgrades_.end()) {
      return it->block;
    }
    return outcome::failure(RuntimeUpgradeTrackerError::NOT_FOUND);
  }

  void RuntimeUpgradeTrackerImpl::subscribeToBlockchainEvents(
      std::shared_ptr<primitives::events::ChainSubscriptionEngine>
          chain_sub_engine,
      std::shared_ptr<const blockchain::BlockTree> block_tree) {
    block_tree_ = std::move(block_tree);
    BOOST_ASSERT(block_tree_ != nullptr);

    chain_subscription_ =
        std::make_shared<primitives::events::ChainEventSubscriber>(
            chain_sub_engine);
    BOOST_ASSERT(chain_subscription_ != nullptr);

    auto chain_subscription_set_id =
        chain_subscription_->generateSubscriptionSetId();
    chain_subscription_->subscribe(
        chain_subscription_set_id,
        primitives::events::ChainEventType::kNewRuntime);
    chain_subscription_->setCallback(
        [this](auto set_id,
               auto &receiver,
               primitives::events::ChainEventType event_type,
               const primitives::events::ChainEventParams &event_params) {
          if (event_type != primitives::events::ChainEventType::kNewRuntime) {
            return;
          }
          auto &block_hash =
              boost::get<primitives::events::NewRuntimeEventParams>(
                  event_params)
                  .get();
          SL_INFO(logger_, "Runtime upgrade at block {}", block_hash.toHex());
          (void)push(block_hash);
        });
  }

  outcome::result<storage::trie::RootHash> RuntimeUpgradeTrackerImpl::push(
      const primitives::BlockHash &hash) {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(hash));
    runtime_upgrades_.emplace_back(primitives::BlockInfo{header.number, hash},
                                   std::move(header.state_root));
    std::sort(runtime_upgrades_.begin(),
              runtime_upgrades_.end(),
              [](const auto &lhs, const auto &rhs) {
                return lhs.block.number < rhs.block.number;
              });
    save();
    return header.state_root;
  }

  void RuntimeUpgradeTrackerImpl::save() {
    auto encoded_res = scale::encode(runtime_upgrades_);
    if (encoded_res.has_value()) {
      auto put_res = storage_->put(storage::kRuntimeHashesLookupKey,
                                   common::Buffer(encoded_res.value()));
      if (not put_res.has_value()) {
        SL_ERROR(logger_,
                 "Could not store hashes of blocks changing runtime: {}",
                 put_res.error().message());
      }
    } else {
      SL_ERROR(logger_,
               "Could not store hashes of blocks changing runtime: {}",
               encoded_res.error().message());
    }
  }
}  // namespace kagome::runtime

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, RuntimeUpgradeTrackerError, e) {
  using E = kagome::runtime::RuntimeUpgradeTrackerError;
  switch (e) {
    case E::NOT_FOUND:
      return "Block hash for the given state not found among runtime upgrades.";
  }
  return "unknown error";
}
