/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_upgrade_tracker_impl.hpp"

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "log/profiling_logger.hpp"
#include "log/trace_macros.hpp"
#include "runtime/common/storage_code_provider.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::runtime {
  outcome::result<std::unique_ptr<RuntimeUpgradeTrackerImpl>>
  RuntimeUpgradeTrackerImpl::create(
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<const primitives::CodeSubstituteBlockIds>
          code_substitutes,
      std::shared_ptr<blockchain::BlockStorage> block_storage) {
    BOOST_ASSERT(header_repo);
    BOOST_ASSERT(storage);
    BOOST_ASSERT(code_substitutes);
    BOOST_ASSERT(block_storage);

    OUTCOME_TRY(encoded_opt,
                storage->getSpace(storage::Space::kDefault)
                    ->tryGet(storage::kRuntimeHashesLookupKey));

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
                                      std::move(saved_data),
                                      std::move(block_storage))};
  }

  RuntimeUpgradeTrackerImpl::RuntimeUpgradeTrackerImpl(
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<const primitives::CodeSubstituteBlockIds>
          code_substitutes,
      std::vector<RuntimeUpgradeData> &&saved_data,
      std::shared_ptr<blockchain::BlockStorage> block_storage)
      : runtime_upgrades_{std::move(saved_data)},
        header_repo_{std::move(header_repo)},
        storage_{storage->getSpace(storage::Space::kDefault)},
        known_code_substitutes_{std::move(code_substitutes)},
        block_storage_{std::move(block_storage)},
        logger_{log::createLogger("StorageCodeProvider", "runtime")} {}

  bool RuntimeUpgradeTrackerImpl::hasCodeSubstitute(
      const kagome::primitives::BlockInfo &block_info) const {
    return known_code_substitutes_->contains(block_info);
  }

  outcome::result<bool> RuntimeUpgradeTrackerImpl::isStateInChain(
      const primitives::BlockInfo &state,
      const primitives::BlockInfo &chain_end) const {
    // if the found state is finalized, it is guaranteed to not belong to a
    // different fork
    primitives::BlockInfo last_finalized;
    auto block_tree = block_tree_.lock();
    if (block_tree) {
      last_finalized = block_tree->getLastFinalized();  // less expensive
    } else {
      OUTCOME_TRY(block_info, block_storage_->getLastFinalized());
      last_finalized = block_info;
    }
    if (last_finalized.number >= state.number) {
      return true;
    }
    // a non-finalized state may belong to a different fork, need to check
    // explicitly (can be expensive if blocks are far apart)
    KAGOME_PROFILE_START(has_direct_chain)
    BOOST_ASSERT(block_tree);
    bool has_direct_chain =
        block_tree->hasDirectChain(state.hash, chain_end.hash);
    KAGOME_PROFILE_END(has_direct_chain)
    return has_direct_chain;
  }

  outcome::result<std::optional<storage::trie::RootHash>>
  RuntimeUpgradeTrackerImpl::findProperFork(
      const primitives::BlockInfo &block,
      std::vector<RuntimeUpgradeData>::const_reverse_iterator latest_upgrade_it)
      const {
    for (; latest_upgrade_it != runtime_upgrades_.rend(); latest_upgrade_it++) {
      OUTCOME_TRY(in_chain, isStateInChain(latest_upgrade_it->block, block));
      if (in_chain) {
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
    if (hasCodeSubstitute(block)) {
      OUTCOME_TRY(push(block.hash));
    }

    // if there are no known blocks with runtime upgrades, we just fall back to
    // returning the state of the current block
    if (runtime_upgrades_.empty()) {
      // even if it doesn't actually upgrade runtime, still a solid source of
      // runtime code
      OUTCOME_TRY(res, push(block.hash));
      SL_DEBUG(
          logger_, "Pick runtime state at block {} for the same block", block);
      return std::move(res.first);
    }

    KAGOME_PROFILE_START(blocks_with_runtime_upgrade_search)
    auto block_number = block.number;
    auto latest_upgrade =
        std::upper_bound(runtime_upgrades_.begin(),
                         runtime_upgrades_.end(),
                         block_number,
                         [](auto block_number, const auto &upgrade_data) {
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
    BOOST_ASSERT(block_tree != nullptr);
    block_tree_ = block_tree;

    chain_subscription_ =
        std::make_shared<primitives::events::ChainEventSubscriber>(
            chain_sub_engine);
    BOOST_ASSERT(chain_subscription_ != nullptr);

    primitives::events::subscribe(
        *chain_subscription_,
        primitives::events::ChainEventType::kNewRuntime,
        [this](const primitives::events::ChainEventParams &event_params) {
          const auto &block_hash =
              boost::get<primitives::events::NewRuntimeEventParams>(
                  event_params)
                  .get();
          auto res = push(block_hash);
          if (res.has_value() and res.value().second) {
            auto header_res = header_repo_->getBlockHeader(block_hash);
            if (header_res.has_value()) {
              auto &header = header_res.value();
              primitives::BlockInfo block_info{header.number, block_hash};
              SL_INFO(logger_, "Runtime upgrade at block {}", block_info);
            }
          }
        });
  }

  outcome::result<std::pair<storage::trie::RootHash, bool>>
  RuntimeUpgradeTrackerImpl::push(const primitives::BlockHash &hash) {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(hash));
    primitives::BlockInfo block_info{header.number, hash};

    bool is_new_upgrade = std::find_if(runtime_upgrades_.begin(),
                                       runtime_upgrades_.end(),
                                       [&](const RuntimeUpgradeData &rud) {
                                         return rud.block == block_info;
                                       })
                       == runtime_upgrades_.end();

    if (is_new_upgrade) {
      runtime_upgrades_.emplace_back(block_info, std::move(header.state_root));

      std::sort(runtime_upgrades_.begin(),
                runtime_upgrades_.end(),
                [](const auto &lhs, const auto &rhs) {
                  return lhs.block.number < rhs.block.number;
                });
      save();
      return std::make_pair(header.state_root, true);
    }

    return std::make_pair(header.state_root, false);
  }

  void RuntimeUpgradeTrackerImpl::save() {
    auto encoded_res = scale::encode(runtime_upgrades_);
    if (encoded_res.has_value()) {
      auto put_res = storage_->put(storage::kRuntimeHashesLookupKey,
                                   common::Buffer(encoded_res.value()));
      if (not put_res.has_value()) {
        SL_ERROR(logger_,
                 "Could not store hashes of blocks changing runtime: {}",
                 put_res.error());
      }
    } else {
      SL_ERROR(logger_,
               "Could not store hashes of blocks changing runtime: {}",
               encoded_res.error());
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
