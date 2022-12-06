/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_manager_impl.hpp"

#include <stack>
#include <unordered_set>

#include <boost/range/adaptor/reversed.hpp>
#include <scale/scale.hpp>

#include "application/app_state_manager.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "consensus/grandpa/authority_manager_error.hpp"
#include "consensus/grandpa/grandpa_digest_observer_error.hpp"
#include "consensus/grandpa/impl/schedule_node.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/runtime_api/grandpa_api.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/trie_batches.hpp"

using kagome::common::Buffer;
using kagome::primitives::AuthoritySetId;

namespace kagome::consensus::grandpa {

  AuthorityManagerImpl::AuthorityManagerImpl(
      Config config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<storage::BufferStorage> persistent_storage,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine)
      : config_{std::move(config)},
        block_tree_(std::move(block_tree)),
        trie_storage_(std::move(trie_storage)),
        grandpa_api_(std::move(grandpa_api)),
        hasher_(std::move(hasher)),
        persistent_storage_{std::move(persistent_storage)},
        header_repo_{std::move(header_repo)},
        chain_sub_([&] {
          BOOST_ASSERT(chain_events_engine != nullptr);
          return std::make_shared<primitives::events::ChainEventSubscriber>(
              chain_events_engine);
        }()),
        logger_{log::createLogger("AuthorityManager", "authority")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(trie_storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(persistent_storage_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);

    BOOST_ASSERT(app_state_manager != nullptr);
    app_state_manager->atPrepare([&] { return prepare(); });
  }

  AuthorityManagerImpl::~AuthorityManagerImpl() {}

  bool AuthorityManagerImpl::prepare() {
    auto load_res = load();
    if (load_res.has_error()) {
      SL_VERBOSE(logger_, "Can not load state: {}", load_res.error());
      return false;
    }

    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kFinalizedHeads);
    chain_sub_->setCallback(
        [wp = weak_from_this()](
            subscription::SubscriptionSetId,
            auto &&,
            primitives::events::ChainEventType type,
            const primitives::events::ChainEventParams &event) {
          if (type == primitives::events::ChainEventType::kFinalizedHeads) {
            if (auto self = wp.lock()) {
              const auto &header =
                  boost::get<primitives::events::HeadsEventParams>(event).get();
              auto hash =
                  self->hasher_->blake2b_256(scale::encode(header).value());

              auto save_res = self->save();
              if (save_res.has_error()) {
                SL_WARN(self->logger_,
                        "Can not save state at finalization: {}",
                        save_res.error());
              }
              self->prune({header.number, hash});
            }
          }
        });

    return true;
  }

  outcome::result<void> AuthorityManagerImpl::load() {
    const auto finalized_block = block_tree_->getLastFinalized();

    // 1. Load last state
    OUTCOME_TRY(encoded_last_state_opt,
                persistent_storage_->tryGet(
                    storage::kAuthorityManagerStateLookupKey("last")));

    if (encoded_last_state_opt.has_value()) {
      auto last_state_res = scale::decode<std::shared_ptr<ScheduleNode>>(
          encoded_last_state_opt.value());

      if (last_state_res.has_value()) {
        auto &last_state = last_state_res.value();
        if (last_state->block.number <= finalized_block.number) {
          root_ = std::move(last_state);
          SL_DEBUG(logger_,
                   "State was initialized by last saved on block {}",
                   root_->block);
        } else {
          SL_WARN(logger_,
                  "Last state not match with last finalized; "
                  "Trying to use savepoint");
        }
      } else {
        SL_WARN(logger_,
                "Can not decode last state: {}; Trying to use savepoint",
                last_state_res.error());
        std::ignore = persistent_storage_->remove(
            storage::kAuthorityManagerStateLookupKey("last"));
      }
    }

    // 2. Load from last control point, if state is still not found
    if (root_ == nullptr) {
      for (auto block_number =
               (finalized_block.number / kSavepointBlockInterval)
               * kSavepointBlockInterval;
           block_number > 0;
           block_number -= kSavepointBlockInterval) {
        OUTCOME_TRY(
            encoded_saved_state_opt,
            persistent_storage_->tryGet(
                storage::kAuthorityManagerStateLookupKey(block_number)));

        if (not encoded_saved_state_opt.has_value()) {
          continue;
        }

        auto saved_state_res = scale::decode<std::shared_ptr<ScheduleNode>>(
            encoded_saved_state_opt.value());

        if (saved_state_res.has_error()) {
          SL_WARN(logger_,
                  "Can not decode state saved on block {}: {}",
                  block_number,
                  saved_state_res.error());
          std::ignore = persistent_storage_->remove(
              storage::kAuthorityManagerStateLookupKey(block_number));
          continue;
        }

        root_ = std::move(saved_state_res.value());
        SL_DEBUG(logger_,
                 "State was initialized by savepoint on block {}",
                 root_->block);
        break;
      }
    }

    // 3. Load state from genesis, if state is still not found
    if (root_ == nullptr) {
      SL_DEBUG(logger_,
               "Appropriate savepoint was not found; Using genesis state");
      auto genesis_hash = block_tree_->getGenesisBlockHash();
      auto authorities_res = grandpa_api_->authorities(genesis_hash);
      if (authorities_res.has_error()) {
        SL_WARN(logger_,
                "Can't get babe config over babe API on genesis block: {}",
                authorities_res.error());
        return authorities_res.as_failure();
      }
      auto &initial_authorities = authorities_res.value();

      root_ =
          ScheduleNode::createAsRoot(std::make_shared<primitives::AuthoritySet>(
                                         0, std::move(initial_authorities)),
                                     {0, genesis_hash});
      SL_DEBUG(logger_, "State was initialized by genesis block");
    }

    BOOST_ASSERT_MSG(root_ != nullptr, "The root must be initialized by now");

    // 4. Apply digests before last finalized
    bool need_to_save = false;
    if (auto n = finalized_block.number - root_->block.number; n > 0) {
      SL_DEBUG(logger_, "Applying digests of {} finalized blocks", n);
    }
    for (auto block_number = root_->block.number + 1;
         block_number <= finalized_block.number;
         ++block_number) {
      auto block_header_res = block_tree_->getBlockHeader(block_number);
      if (block_header_res.has_error()) {
        SL_WARN(logger_,
                "Can't get header of an already finalized block #{}: {}",
                block_number,
                block_header_res.error());
        return block_header_res.as_failure();
      }
      const auto &block_header = block_header_res.value();
      // TODO(xDimon): Would be more efficient to take parent hash of next block
      auto block_hash =
          hasher_->blake2b_256(scale::encode(block_header).value());

      primitives::BlockContext context{.block = {block_number, block_hash},
                                       .header = block_header};

      for (auto &item : block_header.digest) {
        auto res = visit_in_place(
            item,
            [&](const primitives::PreRuntime &msg) -> outcome::result<void> {
              if (msg.consensus_engine_id == primitives::kBabeEngineId) {
                OUTCOME_TRY(
                    digest_item,
                    scale::decode<consensus::babe::BabeBlockHeader>(msg.data));

                return onDigest(context, digest_item);
              }
              return outcome::success();
            },
            [&](const primitives::Consensus &msg) -> outcome::result<void> {
              if (msg.consensus_engine_id == primitives::kGrandpaEngineId) {
                OUTCOME_TRY(digest_item,
                            scale::decode<primitives::GrandpaDigest>(msg.data));

                return onDigest(context, digest_item);
              }
              return outcome::success();
            },
            [](const auto &) { return outcome::success(); });
        if (res.has_error()) {
          SL_WARN(logger_,
                  "Can't apply grandpa digest of finalized block #{}: {}",
                  block_number,
                  res);
          return res.as_failure();
        }
      }

      prune(context.block);

      if (context.block.number % (kSavepointBlockInterval / 10) == 0) {
        // Make savepoint
        auto save_res = save();
        if (save_res.has_error()) {
          SL_WARN(logger_, "Can't re-make savepoint: {}", save_res.error());
        } else {
          need_to_save = false;
        }
      } else {
        need_to_save = true;
      }
    }

    // Save state on finalized part of blockchain
    if (need_to_save) {
      if (auto save_res = save(); save_res.has_error()) {
        SL_WARN(logger_, "Can't re-save state: {}", save_res.error());
      }
    }

    // 4. Collect and apply digests of non-finalized blocks
    auto leaves = block_tree_->getLeaves();
    std::map<primitives::BlockContext,
             std::vector<boost::variant<consensus::babe::BabeBlockHeader,
                                        primitives::GrandpaDigest>>>
        digests;
    // 4.1 Collect digests
    for (auto &leave_hash : leaves) {
      for (auto hash = leave_hash;;) {
        auto block_header_res = block_tree_->getBlockHeader(hash);
        if (block_header_res.has_error()) {
          SL_WARN(logger_,
                  "Can't get header of non-finalized block {}: {}",
                  hash,
                  block_header_res.error());
          return block_header_res.as_failure();
        }
        const auto &block_header = block_header_res.value();

        // This block is finalized
        if (block_header.number <= finalized_block.number) {
          break;
        }

        primitives::BlockContext context{.block = {block_header.number, hash}};

        // This block was meet earlier
        if (digests.find(context) != digests.end()) {
          break;
        }

        auto &digest_of_block = digests[context];

        // Search and collect babe digests
        for (auto &item : block_header.digest) {
          auto res = visit_in_place(
              item,
              [&](const primitives::PreRuntime &msg) -> outcome::result<void> {
                if (msg.consensus_engine_id == primitives::kBabeEngineId) {
                  auto res =
                      scale::decode<consensus::babe::BabeBlockHeader>(msg.data);
                  if (res.has_error()) {
                    return res.as_failure();
                  }
                  const auto &digest_item = res.value();

                  digest_of_block.emplace_back(digest_item);
                }
                return outcome::success();
              },
              [&](const primitives::Consensus &msg) -> outcome::result<void> {
                if (msg.consensus_engine_id == primitives::kBabeEngineId) {
                  auto res = scale::decode<primitives::GrandpaDigest>(msg.data);
                  if (res.has_error()) {
                    return res.as_failure();
                  }
                  const auto &digest_item = res.value();

                  digest_of_block.emplace_back(digest_item);
                }
                return outcome::success();
              },
              [](const auto &) { return outcome::success(); });
          if (res.has_error()) {
            SL_WARN(logger_,
                    "Can't collect babe digest of non-finalized block {}: {}",
                    context.block,
                    res.error());
            return res.as_failure();
          }
        }

        hash = block_header.parent_hash;
      }
    }
    // 4.2 Apply digests
    if (not digests.empty()) {
      SL_DEBUG(logger_,
               "Applying digest of {} non-finalized blocks",
               digests.size());
    }
    for (const auto &[context_tmp, digests_of_block] : digests) {
      const auto &context = context_tmp;
      for (const auto &digest : digests_of_block) {
        auto res = visit_in_place(digest, [&](const auto &digest_item) {
          return onDigest(context, digest_item);
        });
        if (res.has_error()) {
          SL_WARN(logger_,
                  "Can't apply babe digest of non-finalized block {}: {}",
                  context.block,
                  res.error());
          return res.as_failure();
        }
      }
    }

    prune(finalized_block);

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::save() {
    const auto finalized_block = block_tree_->getLastFinalized();

    BOOST_ASSERT(last_saved_state_block_ <= finalized_block.number);

    auto saving_state_node = getNode({.block = finalized_block});
    BOOST_ASSERT_MSG(saving_state_node != nullptr,
                     "Finalized block must have associated node");
    const auto saving_state_block = saving_state_node->block;

    // Does not need to save
    if (last_saved_state_block_ >= saving_state_block.number) {
      return outcome::success();
    }

    const auto last_savepoint =
        (last_saved_state_block_ / kSavepointBlockInterval)
        * kSavepointBlockInterval;

    const auto new_savepoint =
        (saving_state_block.number / kSavepointBlockInterval)
        * kSavepointBlockInterval;

    // It's time to make savepoint
    if (new_savepoint > last_savepoint) {
      auto hash_res = header_repo_->getHashByNumber(new_savepoint);
      if (hash_res.has_value()) {
        primitives::BlockInfo savepoint_block(new_savepoint, hash_res.value());

        auto ancestor_node = getNode({.block = savepoint_block});
        if (ancestor_node != nullptr) {
          auto node = ancestor_node->block == savepoint_block
                          ? ancestor_node
                          : ancestor_node->makeDescendant(
                              savepoint_block, IsBlockFinalized{true});
          auto res = persistent_storage_->put(
              storage::kAuthorityManagerStateLookupKey(new_savepoint),
              storage::Buffer(scale::encode(node).value()));
          if (res.has_error()) {
            SL_WARN(logger_,
                    "Can't make savepoint on block {}: {}",
                    savepoint_block,
                    hash_res.error());
            return res.as_failure();
          }
          SL_DEBUG(logger_, "Savepoint has made on block {}", savepoint_block);
        }
      } else {
        SL_WARN(logger_,
                "Can't take hash of savepoint block {}: {}",
                new_savepoint,
                hash_res.error());
      }
    }

    auto res = persistent_storage_->put(
        storage::kAuthorityManagerStateLookupKey("last"),
        storage::Buffer(scale::encode(saving_state_node).value()));
    if (res.has_error()) {
      SL_WARN(logger_,
              "Can't save last state on block {}: {}",
              saving_state_block,
              res.error());
      return res.as_failure();
    }
    SL_DEBUG(logger_, "Last state has saved on block {}", saving_state_block);

    last_saved_state_block_ = saving_state_block.number;

    return outcome::success();
  }

  primitives::BlockInfo AuthorityManagerImpl::base() const {
    if (not root_) {
      logger_->critical("Authority manager has null root");
      std::terminate();
    }
    return root_->block;
  }

  std::optional<std::shared_ptr<const primitives::AuthoritySet>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &target_block,
                                    IsBlockFinalized finalized) const {
    auto node = getNode({.block = target_block});

    if (node == nullptr) {
      return std::nullopt;
    }

    IsBlockFinalized node_in_finalized_chain =
        node->block == target_block
            ? (bool)finalized
            : node->block.number <= block_tree_->getLastFinalized().number;

    auto adjusted_node =
        node->makeDescendant(target_block, node_in_finalized_chain);

    if (adjusted_node->enabled) {
      // Original authorities
      SL_TRACE(logger_,
               "Pick authority set with id {} for block {}",
               adjusted_node->authorities->id,
               target_block);
      for (auto &authority : adjusted_node->authorities->authorities) {
        SL_TRACE(
            logger_, "Authority {}: {}", authority.id.id, authority.weight);
      }
      return adjusted_node->authorities;
    }

    // Zero-weighted authorities
    auto authorities =
        std::make_shared<primitives::AuthoritySet>(*adjusted_node->authorities);
    std::for_each(authorities->begin(),
                  authorities->end(),
                  [](auto &authority) { authority.weight = 0; });
    return authorities;
  }

  outcome::result<void> AuthorityManagerImpl::applyScheduledChange(
      const primitives::BlockContext &context,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    SL_DEBUG(logger_,
             "Applying scheduled change on block {} to activate at block {}",
             context.block,
             activate_at);
    KAGOME_PROFILE_START(get_appropriate_ancestor)
    auto ancestor_node = getNode(context);
    KAGOME_PROFILE_END(get_appropriate_ancestor)

    if (not ancestor_node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    SL_DEBUG(logger_,
             "Authorities for block {} found on block {} with set id {}",
             context.block,
             ancestor_node->block,
             ancestor_node->authorities->id);

    auto schedule_change = [&](const std::shared_ptr<ScheduleNode> &node)
        -> outcome::result<void> {
      auto new_authorities = std::make_shared<primitives::AuthoritySet>(
          node->authorities->id + 1, authorities);

      node->action =
          ScheduleNode::ScheduledChange{activate_at, new_authorities};

      SL_VERBOSE(
          logger_,
          "Authority set change is scheduled after block #{} (set id={})",
          activate_at,
          new_authorities->id);

      size_t index = 0;
      for (auto &authority : *new_authorities) {
        SL_TRACE(logger_,
                 "New authority ({}/{}): id={} weight={}",
                 ++index,
                 new_authorities->authorities.size(),
                 authority.id.id,
                 authority.weight);
      }

      return outcome::success();
    };

    KAGOME_PROFILE_START(is_ancestor_node_finalized)
    IsBlockFinalized is_ancestor_node_finalized =
        ancestor_node->block == block_tree_->getLastFinalized()
        or directChainExists(ancestor_node->block,
                             block_tree_->getLastFinalized());
    KAGOME_PROFILE_END(is_ancestor_node_finalized)

    // maybe_set contains last planned authority set, if present
    std::optional<std::shared_ptr<const primitives::AuthoritySet>> maybe_set =
        std::nullopt;
    if (not is_ancestor_node_finalized) {
      std::shared_ptr<const ScheduleNode> last_node = ancestor_node;
      while (last_node and last_node != root_) {
        if (const auto *action =
                boost::get<ScheduleNode::ScheduledChange>(&last_node->action);
            action != nullptr) {
          if (context.block.number <= action->applied_block) {
            // It's mean, that new Scheduled Changes would be scheduled before
            // previous is activated. So we ignore it
            return outcome::success();
          }

          if (action->new_authorities->id > ancestor_node->authorities->id) {
            maybe_set = action->new_authorities;
          }
          break;
        }

        last_node = last_node->parent.lock();
      }
    }

    if (ancestor_node->block == context.block) {
      if (maybe_set.has_value()) {
        ancestor_node->authorities = maybe_set.value();
      } else {
        ancestor_node->adjust(is_ancestor_node_finalized);
      }

      OUTCOME_TRY(schedule_change(ancestor_node));
    } else {
      KAGOME_PROFILE_START(make_descendant)
      auto new_node = ancestor_node->makeDescendant(context.block, true);
      KAGOME_PROFILE_END(make_descendant)

      if (maybe_set.has_value()) {
        new_node->authorities = maybe_set.value();
      }

      SL_TRACE(logger_,
               "Make a schedule node for block {}, with actual set id {}",
               context.block,
               new_node->authorities->id);

      KAGOME_PROFILE_START(schedule_change)
      OUTCOME_TRY(schedule_change(new_node));
      KAGOME_PROFILE_END(schedule_change)

      // Reorganize ancestry
      KAGOME_PROFILE_START(reorganize)
      reorganize(ancestor_node, new_node);
      KAGOME_PROFILE_END(reorganize)
    }

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyForcedChange(
      const primitives::BlockContext &context,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber delay_start,
      size_t delay) {
    SL_DEBUG(logger_,
             "Applying forced change (delay start: {}, delay: {}) on block {} "
             "to activate at block {}",
             delay_start,
             delay,
             context.block,
             delay_start + delay);
    if (delay_start < root_->block.number) {
      auto lag = root_->block.number - delay_start;
      if (delay > lag) {
        delay_start = root_->block.number;
        delay -= lag;
      } else {
        delay_start = context.block.number;
        delay = 0;
      }
      SL_DEBUG(
          logger_,
          "Applying forced change on block {} is delayed {} blocks. "
          "Normalized to (delay start: {}, delay: {}) to activate at block {}",
          context.block,
          lag,
          delay_start,
          delay,
          delay_start + delay);
    }
    auto delay_start_hash_res = header_repo_->getHashByNumber(delay_start);
    if (delay_start_hash_res.has_error()) {
      SL_ERROR(logger_, "Failed to obtain hash by number {}", delay_start);
    }
    OUTCOME_TRY(delay_start_hash, delay_start_hash_res);
    auto ancestor_node = getNode({.block = {delay_start, delay_start_hash}});

    if (not ancestor_node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    SL_DEBUG(logger_,
             "Found previous authority change at block {} with set id {}",
             ancestor_node->block,
             ancestor_node->authorities->id);

    auto force_change = [&](const std::shared_ptr<ScheduleNode> &node)
        -> outcome::result<void> {
      auto new_authorities = std::make_shared<primitives::AuthoritySet>(
          node->authorities->id + 1, authorities);

      // Force changes
      if (node->block.number >= delay_start + delay) {
        node->authorities = new_authorities;
        SL_VERBOSE(logger_,
                   "Change has been forced on block #{} (set id={})",
                   delay_start + delay,
                   node->authorities->id);
      } else {
        node->action =
            ScheduleNode::ForcedChange{delay_start, delay, new_authorities};
        SL_VERBOSE(logger_,
                   "Change will be forced on block #{} (set id={})",
                   delay_start + delay,
                   new_authorities->id);
      }

      size_t index = 0;
      for (auto &authority : *new_authorities) {
        SL_TRACE(logger_,
                 "New authority ({}/{}): id={} weight={}",
                 ++index,
                 new_authorities->authorities.size(),
                 authority.id.id,
                 authority.weight);
      }

      return outcome::success();
    };

    auto new_node =
        ancestor_node->makeDescendant({delay_start, delay_start_hash}, true);

    OUTCOME_TRY(force_change(new_node));

    // Reorganize ancestry
    ancestor_node->descendants.clear();
    ancestor_node->descendants.push_back(new_node);
    new_node->descendants.clear();  // reset all pending scheduled changes

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyOnDisabled(
      const primitives::BlockContext &context, uint64_t authority_index) {
    if (!config_.on_disable_enabled) {
      SL_TRACE(logger_, "Ignore 'on disabled' message due to config");
      return outcome::success();
    }
    SL_DEBUG(logger_, "Applying disable authority on block {}", context.block);

    auto node = getNode(context);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    auto disable_authority = [&](const std::shared_ptr<ScheduleNode> &node)
        -> outcome::result<void> {
      // Make changed authorities
      auto new_authority_set =
          std::make_shared<primitives::AuthoritySet>(*node->authorities);

      // Check if index not out of bound
      if (authority_index >= node->authorities->authorities.size()) {
        return GrandpaDigestObserverError::WRONG_AUTHORITY_INDEX;
      }

      new_authority_set->authorities[authority_index].weight = 0;
      node->authorities = std::move(new_authority_set);

      SL_VERBOSE(
          logger_,
          "Authority id={} (index={} in set id={}) is disabled on block #{}",
          node->authorities->authorities[authority_index].id.id,
          authority_index,
          node->authorities->id,
          node->block.number);

      return outcome::success();
    };

    IsBlockFinalized node_in_finalized_chain =
        node->block.number <= block_tree_->getLastFinalized().number;

    if (node->block == context.block) {
      node->adjust(node_in_finalized_chain);
      OUTCOME_TRY(disable_authority(node));
    } else {
      auto new_node =
          node->makeDescendant(context.block, node_in_finalized_chain);

      OUTCOME_TRY(disable_authority(new_node));

      // Reorganize ancestry
      auto descendants = std::move(node->descendants);
      for (auto &descendant : descendants) {
        if (directChainExists(context.block, descendant->block)) {
          // Propagate change to descendants
          if (descendant->authorities == node->authorities) {
            descendant->authorities = new_node->authorities;
          }
          new_node->descendants.emplace_back(std::move(descendant));
        } else {
          node->descendants.emplace_back(std::move(descendant));
        }
      }
      node->descendants.emplace_back(std::move(new_node));
    }

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyPause(
      const primitives::BlockContext &context,
      primitives::BlockNumber activate_at) {
    SL_DEBUG(logger_, "Applying pause on block {}", context.block);

    auto node = getNode(context);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    IsBlockFinalized node_in_finalized_chain =
        node->block.number <= block_tree_->getLastFinalized().number;

    auto new_node =
        node->makeDescendant(context.block, node_in_finalized_chain);

    new_node->action = ScheduleNode::Pause{activate_at};

    SL_VERBOSE(
        logger_, "Scheduled pause after block #{}", new_node->block.number);

    // Reorganize ancestry
    auto descendants = std::move(node->descendants);
    for (auto &descendant : descendants) {
      auto &ancestor =
          context.block.number <= descendant->block.number ? new_node : node;
      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyResume(
      const primitives::BlockContext &context,
      primitives::BlockNumber activate_at) {
    auto node = getNode(context);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    IsBlockFinalized node_in_finalized_chain =
        node->block.number <= block_tree_->getLastFinalized().number;

    auto new_node =
        node->makeDescendant(context.block, node_in_finalized_chain);

    new_node->action = ScheduleNode::Resume{activate_at};

    SL_VERBOSE(
        logger_, "Resuming will be done at block #{}", new_node->block.number);

    // Reorganize ancestry
    reorganize(node, new_node);

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onDigest(
      const primitives::BlockContext &context,
      const consensus::babe::BabeBlockHeader &digest) {
    auto node = getNode(context);

    SL_TRACE(logger_,
             "BabeBlockHeader babe-digest on block {}: "
             "slot {}, authority #{}, {}",
             context.block,
             digest.slot_number,
             digest.authority_index,
             to_string(digest.slotType()));

    if (node->block == context.block) {
      return AuthorityManagerError::BAD_ORDER_OF_DIGEST_ITEM;
    }

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onDigest(
      const primitives::BlockContext &context,
      const primitives::GrandpaDigest &digest) {
    return visit_in_place(
        digest,
        [this, &context](
            const primitives::ScheduledChange &msg) -> outcome::result<void> {
          return applyScheduledChange(
              context,
              msg.authorities,
              context.block.number + msg.subchain_length);
        },
        [this, &context](const primitives::ForcedChange &msg) {
          return applyForcedChange(
              context, msg.authorities, msg.delay_start, msg.subchain_length);
        },
        [this, &context](const primitives::OnDisabled &msg) {
          SL_DEBUG(logger_, "OnDisabled {}", msg.authority_index);
          return applyOnDisabled(context, msg.authority_index);
        },
        [this, &context](const primitives::Pause &msg) {
          SL_DEBUG(logger_, "Pause {}", msg.subchain_length);
          return applyPause(context,
                            context.block.number + msg.subchain_length);
        },
        [this, &context](const primitives::Resume &msg) {
          SL_DEBUG(logger_, "Resume {}", msg.subchain_length);
          return applyResume(context,
                             context.block.number + msg.subchain_length);
        },
        [](auto &) {
          return GrandpaDigestObserverError::UNSUPPORTED_MESSAGE_TYPE;
        });
  }

  void AuthorityManagerImpl::prune(const primitives::BlockInfo &block) {
    if (block == root_->block) {
      return;
    }

    if (block.number < root_->block.number) {
      return;
    }

    auto node = getNode({.block = block});

    if (not node) {
      return;
    }

    if (node->block == block) {
      // Rebase
      root_ = std::move(node);

    } else {
      // Reorganize ancestry
      auto new_node = node->makeDescendant(block, true);
      auto descendants = std::move(node->descendants);
      for (auto &descendant : descendants) {
        if (directChainExists(block, descendant->block)) {
          new_node->descendants.emplace_back(std::move(descendant));
        }
      }

      root_ = std::move(new_node);
    }

    SL_TRACE(logger_, "Prune authority manager upto block {}", block);
  }

  std::shared_ptr<ScheduleNode> AuthorityManagerImpl::getNode(
      const primitives::BlockContext &context) const {
    BOOST_ASSERT(root_ != nullptr);

    // Lazy getter of direct chain best block ('cause it may be not used)
    auto get_block =
        [&, block = std::optional<primitives::BlockInfo>()]() mutable {
          if (not block.has_value()) {
            if (context.header.has_value()) {
              const auto &header = context.header.value().get();
              block.emplace(header.number - 1, header.parent_hash);
            } else {
              block.emplace(context.block);
            }
          }
          return block.value();
        };

    // Target block is not descendant of the current root
    if (root_->block.number > context.block.number
        || (root_->block != context.block
            && not directChainExists(root_->block, get_block()))) {
      return nullptr;
    }

    std::shared_ptr<ScheduleNode> ancestor = root_;
    while (ancestor->block != context.block) {
      bool goto_next_generation = false;
      for (const auto &node : ancestor->descendants) {
        if (node->block == context.block) {
          return node;
        }
        if (directChainExists(node->block, get_block())) {
          ancestor = node;
          goto_next_generation = true;
          break;
        }
      }
      if (not goto_next_generation) {
        break;
      }
    }
    return ancestor;
  }

  bool AuthorityManagerImpl::directChainExists(
      const primitives::BlockInfo &ancestor,
      const primitives::BlockInfo &descendant) const {
    SL_TRACE(logger_,
             "Looking if direct chain exists between {} and {}",
             ancestor,
             descendant);
    KAGOME_PROFILE_START(direct_chain_exists)
    // Check if it's one-block chain
    if (ancestor == descendant) {
      return true;
    }
    // Any block is descendant of genesis
    if (ancestor.number == 0) {
      return true;
    }
    // No direct chain if order is wrong
    if (ancestor.number > descendant.number) {
      return false;
    }
    auto result = block_tree_->hasDirectChain(ancestor.hash, descendant.hash);
    KAGOME_PROFILE_END(direct_chain_exists)
    return result;
  }

  void AuthorityManagerImpl::reorganize(
      std::shared_ptr<ScheduleNode> node,
      std::shared_ptr<ScheduleNode> new_node) {
    auto descendants = std::move(node->descendants);
    for (auto &descendant : descendants) {
      auto &ancestor =
          new_node->block.number < descendant->block.number ? new_node : node;

      // Apply if delay will be passed for descendant
      if (auto *forced_change =
              boost::get<ScheduleNode::ForcedChange>(&ancestor->action)) {
        if (descendant->block.number
            >= forced_change->delay_start + forced_change->delay_length) {
          descendant->authorities = forced_change->new_authorities;
          descendant->action = ScheduleNode::NoAction{};
        }
      }
      if (auto *resume = boost::get<ScheduleNode::Resume>(&ancestor->action)) {
        if (descendant->block.number >= resume->applied_block) {
          descendant->enabled = true;
          descendant->action = ScheduleNode::NoAction{};
        }
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));
  }

  void AuthorityManagerImpl::cancel(const primitives::BlockInfo &block) {
    auto ancestor = getNode({.block = block});

    if (ancestor == nullptr) {
      SL_TRACE(logger_, "Can't remove node of block {}: no ancestor", block);
      return;
    }

    if (ancestor == root_) {
      // Can't remove root
      SL_TRACE(logger_, "Can't remove node of block {}: it is root", block);
      return;
    }

    if (ancestor->block == block) {
      ancestor = std::const_pointer_cast<ScheduleNode>(ancestor->parent.lock());
      BOOST_ASSERT_MSG(ancestor != nullptr, "Non root node must have a parent");
    }

    auto it = std::find_if(ancestor->descendants.begin(),
                           ancestor->descendants.end(),
                           [&block](std::shared_ptr<ScheduleNode> node) {
                             return node->block == block;
                           });

    if (it != ancestor->descendants.end()) {
      if (not(*it)->descendants.empty()) {
        // Has descendants - is not a leaf
        SL_TRACE(logger_,
                 "Can't remove node of block {}: "
                 "not found such descendant of ancestor",
                 block);
        return;
      }

      ancestor->descendants.erase(it);
      SL_DEBUG(logger_, "Node of block {} has removed", block);
    }
  }
}  // namespace kagome::consensus::grandpa
