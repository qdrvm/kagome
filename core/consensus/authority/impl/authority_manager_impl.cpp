/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

#include <stack>
#include <unordered_set>

#include <boost/range/adaptor/reversed.hpp>
#include <scale/scale.hpp>

#include "application/app_state_manager.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "consensus/authority/authority_manager_error.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"
#include "consensus/authority/impl/schedule_node.hpp"
#include "crypto/hasher.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/runtime_api/grandpa_api.hpp"
#include "storage/trie/trie_storage.hpp"

using kagome::common::Buffer;
using kagome::primitives::AuthoritySetId;

namespace kagome::authority {

  AuthorityManagerImpl::AuthorityManagerImpl(
      Config config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<storage::BufferStorage> persistent_storage,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
      : config_{std::move(config)},
        block_tree_(std::move(block_tree)),
        trie_storage_(std::move(trie_storage)),
        grandpa_api_(std::move(grandpa_api)),
        hasher_(std::move(hasher)),
        persistent_storage_{std::move(persistent_storage)},
        header_repo_{std::move(header_repo)},
        log_{log::createLogger("AuthorityManager", "authority")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(trie_storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(persistent_storage_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);

    BOOST_ASSERT(app_state_manager != nullptr);
    app_state_manager->atPrepare([&] { return prepare(); });
  }

  struct ConsensusMessages {
    primitives::BlockInfo block;
    primitives::Consensus message;
  };

  outcome::result<std::stack<ConsensusMessages>> collectMsgsFromNonFinalBlocks(
      blockchain::BlockTree const &block_tree,
      primitives::BlockHash const &finalized_block_hash) {
    std::stack<ConsensusMessages> collected;

    std::unordered_set<primitives::BlockHash> observed;
    for (auto &leaf : block_tree.getLeaves()) {
      for (auto hash = leaf;;) {
        if (hash == finalized_block_hash) {
          break;
        }

        // if we already checked this block (and thus the rest of the branch)
        if (not observed.emplace(hash).second) {
          break;
        }

        // just obtained from block tree
        auto header = block_tree.getBlockHeader(hash).value();

        // observe possible changes of authorities
        for (auto &digest_item : boost::adaptors::reverse(header.digest)) {
          visit_in_place(
              digest_item,
              [&](const primitives::Consensus &consensus_message) {
                collected.emplace(ConsensusMessages{
                    primitives::BlockInfo(header.number, hash),
                    consensus_message});
              },
              [](const auto &) {});
        }

        hash = header.parent_hash;
      }
    }
    return collected;
  }

  outcome::result<std::optional<AuthoritySetId>> fetchSetIdFromTrieStorage(
      storage::trie::TrieStorage const &trie_storage,
      crypto::Hasher const &hasher,
      primitives::BlockHeader const &header) {
    OUTCOME_TRY(batch, trie_storage.getEphemeralBatchAt(header.state_root));

    std::optional<AuthoritySetId> set_id_opt;
    auto current_set_id_keypart =
        hasher.twox_128(Buffer::fromString("CurrentSetId"));
    for (auto prefix : {"GrandpaFinality", "Grandpa"}) {
      auto prefix_key_part = hasher.twox_128(Buffer::fromString(prefix));
      auto set_id_key =
          Buffer().put(prefix_key_part).put(current_set_id_keypart);

      OUTCOME_TRY(val_opt, batch->tryGet(set_id_key));
      if (val_opt.has_value()) {
        auto &val = val_opt.value();
        set_id_opt.emplace(scale::decode<AuthoritySetId>(val.get()).value());
        break;
      }
    }
    return set_id_opt;
  }

  static const common::Buffer kScheduleGraphRootKey =
      common::Buffer::fromString(":authority_manager:schedule_graph_root");

  outcome::result<std::optional<std::unique_ptr<ScheduleNode>>>
  fetchScheduleGraphRoot(storage::BufferStorage const &storage) {
    OUTCOME_TRY(opt_root, storage.tryLoad(kScheduleGraphRootKey));
    if (!opt_root) return std::nullopt;
    auto &encoded_root = opt_root.value();
    OUTCOME_TRY(root, scale::decode<ScheduleNode>(encoded_root));
    return std::make_unique<ScheduleNode>(std::move(root));
  }

  outcome::result<void> storeScheduleGraphRoot(storage::BufferStorage &storage,
                                               ScheduleNode const &root) {
    OUTCOME_TRY(enc_root, scale::encode(root));
    OUTCOME_TRY(storage.put(kScheduleGraphRootKey,
                            common::Buffer{std::move(enc_root)}));
    return outcome::success();
  }

  outcome::result<void> clearScheduleGraphRoot(
      storage::BufferStorage &storage) {
    OUTCOME_TRY(storage.remove(kScheduleGraphRootKey));
    return outcome::success();
  }

  static const common::Buffer kForcedSetIdListKey =
      common::Buffer::fromString(":authority_manager:forced_set_id_list");

  /**
   * Collect all consensus messages found in finalized block starting from
   * {@param finalized_block_hash} and until an authority set change is reached.
   * @param collected_msgs - output stack of msgs
   * @param finalized_block_hash - last finalized block
   * @param block_tree - block tree
   * @param log - logger
   * @return block significant to make root node (or error)
   */
  outcome::result<primitives::BlockInfo>
  collectConsensusMsgsUntilNearestSetChangeTo(
      std::stack<ConsensusMessages> &collected_msgs,
      const primitives::BlockInfo &finalized_block,
      const blockchain::BlockTree &block_tree,
      log::Logger &log) {
    bool found_set_change = false;
    bool is_unapplied_change = false;

    for (auto hash = finalized_block.hash; !found_set_change;) {
      auto header_res = block_tree.getBlockHeader(hash);
      if (!header_res) {
        SL_ERROR(
            log, "Failed to obtain the last finalized block header {}", hash);
      }
      OUTCOME_TRY(header, header_res);

      if (header.number == 0) {
        found_set_change = true;
      } else {
        // observe possible changes of authorities
        for (auto &digest_item : boost::adaptors::reverse(header.digest)) {
          visit_in_place(
              digest_item,
              [&](const primitives::Consensus &consensus_message) {
                const bool is_grandpa = consensus_message.consensus_engine_id
                                        == primitives::kGrandpaEngineId;
                if (not is_grandpa) {
                  return;
                }

                auto decoded_res = consensus_message.decode();
                if (decoded_res.has_error()) {
                  log->critical("Error decoding consensus message: {}",
                                decoded_res.error().message());
                }
                auto &grandpa_digest = decoded_res.value().asGrandpaDigest();

                auto scheduled_change =
                    boost::get<primitives::ScheduledChange>(&grandpa_digest);
                if (scheduled_change != nullptr) {
                  found_set_change = true;
                  is_unapplied_change =
                      header.number + scheduled_change->subchain_length
                      >= finalized_block.number;
                  if (is_unapplied_change) {
                    collected_msgs.emplace(ConsensusMessages{
                        primitives::BlockInfo(header.number, hash),
                        consensus_message});
                  }
                  return;
                }

                auto forced_change =
                    boost::get<primitives::ForcedChange>(&grandpa_digest);
                if (forced_change != nullptr) {
                  found_set_change = true;
                  is_unapplied_change =
                      header.number + forced_change->subchain_length
                      >= finalized_block.number;
                  if (is_unapplied_change) {
                    collected_msgs.emplace(ConsensusMessages{
                        primitives::BlockInfo(header.number, hash),
                        consensus_message});
                  }
                  return;
                }
              },
              [](const auto &...) {});  // Other variants are ignored
        }
      }

      if (found_set_change) {
        auto block = is_unapplied_change ? primitives::BlockInfo(
                         header.number - 1, header.parent_hash)
                                         : finalized_block;

        return block;
      } else {
        hash = header.parent_hash;
      }
    }
    BOOST_UNREACHABLE_RETURN({})
  }

#define CONCAT(first, second) CONCAT_UTIL(first, second)
#define CONCAT_UTIL(first, second) first##second
#define UNIQUE_NAME(tag) CONCAT(tag, __LINE__)

#define PREPARE_TRY_VOID(expr_res, error_msg, ...) \
  auto &&UNIQUE_NAME(expr_r_) = (expr_res);        \
  if (UNIQUE_NAME(expr_r_).has_error()) {          \
    auto &error = UNIQUE_NAME(expr_r_).error();    \
    log_->critical(error_msg, __VA_ARGS__);        \
    return false;                                  \
  }

#define PREPARE_TRY(val, expr_res, error_msg, ...)    \
  PREPARE_TRY_VOID(expr_res, error_msg, __VA_ARGS__); \
  auto &val = UNIQUE_NAME(expr_r_).value();

  bool AuthorityManagerImpl::prepare() {
    const auto finalized_block = block_tree_->getLastFinalized();

    PREPARE_TRY(
        collected_msgs,
        collectMsgsFromNonFinalBlocks(*block_tree_, finalized_block.hash),
        "Error collecting consensus messages from non-finalized blocks: {}",
        error.message());

    PREPARE_TRY(opt_root,
                fetchScheduleGraphRoot(*persistent_storage_),
                "Error fetching authority set from persistent storage: {}",
                error.message());
    auto last_finalized_block = block_tree_->getLastFinalized();
    if (opt_root
        && opt_root.value()->current_block.number
               <= last_finalized_block.number) {
      root_ = std::move(opt_root.value());
      SL_TRACE(log_,
               "Fetched authority set graph root from database with id {}",
               root_->current_authorities->id);

    } else if (last_finalized_block.number == 0) {
      auto genesis_hash = block_tree_->getGenesisBlockHash();
      PREPARE_TRY(initial_authorities,
                  grandpa_api_->authorities(genesis_hash),
                  "Can't get grandpa authorities for genesis block: {}",
                  error.message());
      root_ = authority::ScheduleNode::createAsRoot(
          std::make_shared<primitives::AuthoritySet>(
              0, std::move(initial_authorities)),
          {0, genesis_hash});
    } else {
      SL_WARN(
          log_,
          "Storage does not contain valid info about the root authority set; "
          "Fall back to obtaining it from the runtime storage (which may "
          "fail after a forced authority change happened on chain)");
      PREPARE_TRY(
          graph_root_block,
          collectConsensusMsgsUntilNearestSetChangeTo(
              collected_msgs, finalized_block, *block_tree_, log_),
          "Error collecting consensus messages from finalized blocks: {}",
          error.message());
      PREPARE_TRY(authorities,
                  grandpa_api_->authorities(graph_root_block.hash),
                  "Can't get grandpa authorities for block {}: {}",
                  graph_root_block,
                  error.message());

      PREPARE_TRY(header,
                  block_tree_->getBlockHeader(graph_root_block.hash),
                  "Can't get header of block {}: {}",
                  graph_root_block,
                  error.message());
      AuthoritySetId set_id{};
      auto set_id_res = grandpa_api_->current_set_id(graph_root_block.hash);
      if (set_id_res) {
        set_id = set_id_res.value();
      } else {
        PREPARE_TRY(
            set_id_,
            fetchSetIdFromTrieStorage(*trie_storage_, *hasher_, header),
            "Failed to fetch authority set id from storage for block {}: {}",
            graph_root_block,
            error.message());
        set_id = set_id_.value();
      }
      auto authority_set = std::make_shared<primitives::AuthoritySet>(
          set_id, std::move(authorities));
      root_ = authority::ScheduleNode::createAsRoot(authority_set,
                                                    graph_root_block);

      PREPARE_TRY_VOID(storeScheduleGraphRoot(*persistent_storage_, *root_),
                       "Failed to store schedule graph root: {}",
                       error.message());
      SL_TRACE(log_,
               "Create authority set graph root with id {}, taken from runtime "
               "storage",
               root_->current_authorities->id);
    }

    while (not collected_msgs.empty()) {
      const auto &args = collected_msgs.top();
      PREPARE_TRY_VOID(onConsensus(args.block, args.message),
                       "Can't apply previous consensus message: {}",
                       error.message());

      collected_msgs.pop();
    }

    // prune to reorganize collected changes
    prune(finalized_block);

    SL_DEBUG(log_,
             "Actual grandpa authority set (id={}):",
             root_->current_authorities->id);
    size_t index = 0;
    for (const auto &authority : *root_->current_authorities) {
      SL_TRACE(log_,
               "{}/{}: id={} weight={}",
               ++index,
               root_->current_authorities->authorities.size(),
               authority.id.id,
               authority.weight);
    }

    return true;
  }

#undef PREPARE_TRY_VOID
#undef PREPARE_TRY

  outcome::result<void> AuthorityManagerImpl::recalculateStoredState(
      primitives::BlockNumber last_finalized_number) {
    auto genesis_hash = block_tree_->getGenesisBlockHash();

    OUTCOME_TRY(initial_authorities, grandpa_api_->authorities(genesis_hash));

    root_ = ScheduleNode::createAsRoot(
        std::make_shared<primitives::AuthoritySet>(0, initial_authorities),
        {0, genesis_hash});

    // if state is pruned
    if (header_repo_->getBlockHeader(1).has_error()) {
      SL_WARN(log_,
              "Can't recalculate authority set id on a prune state, fall back "
              "to fetching from runtime");
      return clearScheduleGraphRoot(*persistent_storage_);
    }

    for (primitives::BlockNumber number = 0; number <= last_finalized_number;
         number++) {
      auto start = std::chrono::steady_clock::now();
      auto header_res = header_repo_->getBlockHeader(number);
      if(!header_res) continue; // Temporary workaround about the justification pruning bug
      auto& header = header_res.value();

      OUTCOME_TRY(hash, header_repo_->getHashByNumber(number));
      primitives::BlockInfo info{number, hash};

      for (auto &msg : header.digest) {
        if (auto consensus_msg = boost::get<primitives::Consensus>(&msg);
            consensus_msg != nullptr) {
          onConsensus(info, *consensus_msg).value();
        }
      }
      auto justification_res = block_tree_->getBlockJustification(hash);
      if (justification_res.has_value()) prune(info);
      auto end = std::chrono::steady_clock::now();
      SL_TRACE(
          log_,
          "Process block #{} in {} ms",
          number,
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count());
    }
    return outcome::success();
  }

  primitives::BlockInfo AuthorityManagerImpl::base() const {
    if (not root_) {
      log_->critical("Authority manager has null root");
      std::terminate();
    }
    return root_->current_block;
  }

  std::optional<std::shared_ptr<const primitives::AuthoritySet>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &target_block,
                                    IsBlockFinalized finalized) const {
    auto node = getAppropriateAncestor(target_block);

    if (node == nullptr) {
      return std::nullopt;
    }

    IsBlockFinalized node_in_finalized_chain =
        node->current_block == target_block
            ? (bool)finalized
            : node->current_block.number
                  <= block_tree_->getLastFinalized().number;
    if (target_block.number == 1514496) {
      node_in_finalized_chain = true;
    }

    auto adjusted_node =
        node->makeDescendant(target_block, node_in_finalized_chain);

    if (adjusted_node->enabled) {
      // Original authorities
      SL_DEBUG(log_,
               "Pick authority set with id {} for block {}",
               adjusted_node->current_authorities->id,
               target_block);
      // Since getAppropriateAncestor worked normally on this block
      auto header = block_tree_->getBlockHeader(target_block.hash).value();
      auto id_from_storage =
          fetchSetIdFromTrieStorage(*trie_storage_, *hasher_, header)
              .value()
              .value();
      SL_DEBUG(
          log_, "Pick authority set id from trie storage: {}", id_from_storage);
      return adjusted_node->current_authorities;
    }

    // Zero-weighted authorities
    auto authorities = std::make_shared<primitives::AuthoritySet>(
        *adjusted_node->current_authorities);
    std::for_each(authorities->begin(),
                  authorities->end(),
                  [](auto &authority) { authority.weight = 0; });
    return authorities;
  }

  outcome::result<void> AuthorityManagerImpl::applyScheduledChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    SL_DEBUG(log_,
             "Applying scheduled change on block {} to activate at block {}",
             block,
             activate_at);
    KAGOME_PROFILE_START(get_appropriate_ancestor)
    auto ancestor_node = getAppropriateAncestor(block);
    KAGOME_PROFILE_END(get_appropriate_ancestor)

    if (not ancestor_node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    SL_DEBUG(log_,
             "Authorities for block {} found on block {} with set id {}",
             block,
             ancestor_node->current_block,
             ancestor_node->current_authorities->id);

    auto schedule_change = [&](const std::shared_ptr<ScheduleNode> &node)
        -> outcome::result<void> {
      auto new_authorities = std::make_shared<primitives::AuthoritySet>(
          node->current_authorities->id + 1, authorities);

      node->action =
          ScheduleNode::ScheduledChange{activate_at, new_authorities};

      SL_VERBOSE(log_,
                 "Change is scheduled after block #{} (set id={})",
                 activate_at,
                 new_authorities->id);

      size_t index = 0;
      for (auto &authority : *new_authorities) {
        SL_DEBUG(log_,
                 "New authority ({}/{}): id={} weight={}",
                 ++index,
                 new_authorities->authorities.size(),
                 authority.id.id,
                 authority.weight);
      }

      return outcome::success();
    };

    IsBlockFinalized is_ancestor_node_finalized = true;

    if (ancestor_node->current_block == block) {
      ancestor_node->adjust(is_ancestor_node_finalized);

      OUTCOME_TRY(schedule_change(ancestor_node));
    } else {
      KAGOME_PROFILE_START(make_descendant)
      auto new_node =
          ancestor_node->makeDescendant(block, is_ancestor_node_finalized);
      KAGOME_PROFILE_END(make_descendant)
      SL_DEBUG(log_,
               "Make a schedule node for block {}, with actual set id {}",
               block,
               new_node->current_authorities->id);

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
      const primitives::BlockInfo &current_block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber delay_start,
      size_t delay) {
    SL_DEBUG(log_,
             "Applying forced change (delay start: {}, delay: {}) on block {} "
             "to activate at block {}",
             delay_start,
             delay,
             current_block,
             delay_start + delay);
    auto delay_start_header_res =
                block_tree_->getBlockHeader(delay_start + 1);
    if (delay_start_header_res.has_error()) {
      SL_ERROR(log_, "Failed to obtain header by number {}", delay_start + 1);
    }
    OUTCOME_TRY(delay_start_header, delay_start_header_res);
    auto ancestor_node =
        getAppropriateAncestor({delay_start, delay_start_header.parent_hash});

    if (not ancestor_node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    SL_DEBUG(log_,
             "Found previous authority change at block {} with set id {}",
             ancestor_node->current_block,
             ancestor_node->current_authorities->id);

    auto force_change = [&](const std::shared_ptr<ScheduleNode> &node)
        -> outcome::result<void> {
      auto new_authorities = std::make_shared<primitives::AuthoritySet>(
          node->current_authorities->id + 1, authorities);

      // Force changes
      if (node->current_block.number >= delay_start + delay) {
        node->current_authorities = new_authorities;
        SL_VERBOSE(log_,
                   "Change has been forced on block #{} (set id={})",
                   delay_start + delay,
                   node->current_authorities->id);
      } else {
        node->action =
            ScheduleNode::ForcedChange{delay_start, delay, new_authorities};
        SL_VERBOSE(log_,
                   "Change will be forced on block #{} (set id={})",
                   delay_start + delay,
                   new_authorities->id);
      }

      size_t index = 0;
      for (auto &authority : *new_authorities) {
        SL_DEBUG(log_,
                 "New authority ({}/{}): id={} weight={}",
                 ++index,
                 new_authorities->authorities.size(),
                 authority.id.id,
                 authority.weight);
      }

      return outcome::success();
    };

    auto new_node = ancestor_node->makeDescendant(
        {delay_start, delay_start_header.parent_hash}, true);

    OUTCOME_TRY(force_change(new_node));

    // Reorganize ancestry
    ancestor_node->descendants.clear();
    ancestor_node->descendants.push_back(new_node);
    new_node->descendants.clear();  // reset all pending scheduled changes

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyOnDisabled(
      const primitives::BlockInfo &block, uint64_t authority_index) {
    if (!config_.on_disable_enabled) {
      SL_TRACE(log_, "Ignore 'on disabled' message due to config");
      return outcome::success();
    }
    SL_DEBUG(log_, "Applying disable authority on block {}", block);

    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    auto disable_authority = [&](const std::shared_ptr<ScheduleNode> &node)
        -> outcome::result<void> {
      // Make changed authorities
      auto new_authority_set = std::make_shared<primitives::AuthoritySet>(
          *node->current_authorities);

      // Check if index not out of bound
      if (authority_index >= node->current_authorities->authorities.size()) {
        return AuthorityUpdateObserverError::WRONG_AUTHORITY_INDEX;
      }

      new_authority_set->authorities[authority_index].weight = 0;
      node->current_authorities = std::move(new_authority_set);

      SL_VERBOSE(
          log_,
          "Authority id={} (index={} in set id={}) is disabled on block #{}",
          node->current_authorities->authorities[authority_index].id.id,
          authority_index,
          node->current_authorities->id,
          node->current_block.number);

      return outcome::success();
    };

    IsBlockFinalized node_in_finalized_chain =
        node->current_block.number <= block_tree_->getLastFinalized().number;

    if (node->current_block == block) {
      node->adjust(node_in_finalized_chain);
      OUTCOME_TRY(disable_authority(node));
    } else {
      auto new_node = node->makeDescendant(block, node_in_finalized_chain);

      OUTCOME_TRY(disable_authority(new_node));

      // Reorganize ancestry
      auto descendants = std::move(node->descendants);
      for (auto &descendant : descendants) {
        if (directChainExists(block, descendant->current_block)) {
          // Propagate change to descendants
          if (descendant->current_authorities == node->current_authorities) {
            descendant->current_authorities = new_node->current_authorities;
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
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    SL_DEBUG(log_, "Applying pause on block {}", block);

    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    IsBlockFinalized node_in_finalized_chain =
        node->current_block.number <= block_tree_->getLastFinalized().number;

    auto new_node = node->makeDescendant(block, node_in_finalized_chain);

    new_node->action = ScheduleNode::Pause{activate_at};

    SL_VERBOSE(log_,
               "Scheduled pause after block #{}",
               new_node->current_block.number);

    // Reorganize ancestry
    auto descendants = std::move(node->descendants);
    for (auto &descendant : descendants) {
      auto &ancestor =
          block.number <= descendant->current_block.number ? new_node : node;
      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyResume(
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    IsBlockFinalized node_in_finalized_chain =
        node->current_block.number <= block_tree_->getLastFinalized().number;

    auto new_node = node->makeDescendant(block, node_in_finalized_chain);

    new_node->action = ScheduleNode::Resume{activate_at};

    SL_VERBOSE(log_,
               "Resuming will be done at block #{}",
               new_node->current_block.number);

    // Reorganize ancestry
    reorganize(node, new_node);

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onConsensus(
      const primitives::BlockInfo &block,
      const primitives::Consensus &message) {
    if (message.consensus_engine_id == primitives::kGrandpaEngineId) {
      SL_TRACE(log_,
               "Apply consensus message from block {}, engine {}",
               block,
               message.consensus_engine_id.toString());

      OUTCOME_TRY(decoded, message.decode());
      return visit_in_place(
          decoded.asGrandpaDigest(),
          [this, &block](
              const primitives::ScheduledChange &msg) -> outcome::result<void> {
            return applyScheduledChange(
                block, msg.authorities, block.number + msg.subchain_length);
          },
          [this, &block](const primitives::ForcedChange &msg) {
            return applyForcedChange(
                block, msg.authorities, msg.delay_start, msg.subchain_length);
          },
          [this, &block](const primitives::OnDisabled &msg) {
            return applyOnDisabled(block, msg.authority_index);
          },
          [this, &block](const primitives::Pause &msg) {
            return applyPause(block, block.number + msg.subchain_length);
          },
          [this, &block](const primitives::Resume &msg) {
            return applyResume(block, block.number + msg.subchain_length);
          },
          [](auto &) {
            return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
          });
    } else if (message.consensus_engine_id == primitives::kBabeEngineId) {
      // ignore
      return outcome::success();

    } else {
      SL_DEBUG(log_,
               "Unknown consensus engine id in block {}: {}",
               block,
               message.consensus_engine_id.toString());
      return outcome::success();
    }
  }

  void AuthorityManagerImpl::prune(const primitives::BlockInfo &block) {
    if (block == root_->current_block) {
      return;
    }

    if (block.number < root_->current_block.number) {
      return;
    }

    auto node = getAppropriateAncestor(block);

    if (not node) {
      return;
    }

    if (node->current_block == block) {
      // Rebase
      root_ = std::move(node);

    } else {
      // Reorganize ancestry
      auto new_node = node->makeDescendant(block, true);
      auto descendants = std::move(node->descendants);
      for (auto &descendant : descendants) {
        if (directChainExists(block, descendant->current_block)) {
          new_node->descendants.emplace_back(std::move(descendant));
        }
      }

      root_ = std::move(new_node);
    }
    storeScheduleGraphRoot(*persistent_storage_, *root_).value();

    SL_VERBOSE(log_, "Prune authority manager upto block {}", block);
  }

  std::shared_ptr<ScheduleNode> AuthorityManagerImpl::getAppropriateAncestor(
      const primitives::BlockInfo &block) const {
    BOOST_ASSERT(root_ != nullptr);

    // Target block is not descendant of the current root
    if (root_->current_block.number > block.number
        || (root_->current_block != block
            && not directChainExists(root_->current_block, block))) {
      return nullptr;
    }
    std::shared_ptr<ScheduleNode> ancestor = root_;
    while (ancestor->current_block != block) {
      bool goto_next_generation = false;
      for (const auto &node : ancestor->descendants) {
        if (node->current_block == block
            || directChainExists(node->current_block, block)) {
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
    SL_TRACE(log_,
             "Looking if direct chain exists between {} and {}",
             ancestor,
             descendant);
    KAGOME_PROFILE_START(direct_chain_exists)
    // Any block is descendant of genesis
    if (ancestor.number <= 1 && ancestor.number < descendant.number) {
      return true;
    }
    auto result =
        ancestor.number < descendant.number
        && block_tree_->hasDirectChain(ancestor.hash, descendant.hash);
    KAGOME_PROFILE_END(direct_chain_exists)
    return result;
  }

  void AuthorityManagerImpl::reorganize(
      std::shared_ptr<ScheduleNode> node,
      std::shared_ptr<ScheduleNode> new_node) {
    auto descendants = std::move(node->descendants);
    for (auto &descendant : descendants) {
      auto &ancestor =
          new_node->current_block.number < descendant->current_block.number
              ? new_node
              : node;

      // Apply if delay will be passed for descendant
      if (auto *forced_change =
              boost::get<ScheduleNode::ForcedChange>(&ancestor->action)) {
        if (descendant->current_block.number
            >= forced_change->delay_start + forced_change->delay_length) {
          descendant->current_authorities = forced_change->new_authorities;
          descendant->action = ScheduleNode::NoAction{};
        }
      }
      if (auto *resume = boost::get<ScheduleNode::Resume>(&ancestor->action)) {
        if (descendant->current_block.number >= resume->applied_block) {
          descendant->enabled = true;
          descendant->action = ScheduleNode::NoAction{};
        }
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));
  }

  void AuthorityManagerImpl::cancel(const primitives::BlockInfo &block) {
    auto ancestor = getAppropriateAncestor(block);

    if (ancestor == nullptr) {
      SL_TRACE(log_, "No scheduled changes on block {}: no ancestor", block);
      return;
    }

    if (ancestor == root_) {
      // Can't remove root
      SL_TRACE(log_,
               "Can't cancel scheduled changes on block {}: it is root",
               block);
      return;
    }

    if (ancestor->current_block == block) {
      ancestor = std::const_pointer_cast<ScheduleNode>(ancestor->parent.lock());
    }

    auto it = std::find_if(ancestor->descendants.begin(),
                           ancestor->descendants.end(),
                           [&block](std::shared_ptr<ScheduleNode> node) {
                             return node->current_block == block;
                           });

    if (it != ancestor->descendants.end()) {
      if (not(*it)->descendants.empty()) {
        // Has descendants - is not a leaf
        SL_TRACE(log_, "No scheduled changes on block {}: not found", block);
        return;
      }

      SL_DEBUG(log_, "Scheduled changes on block {} has removed", block);
      ancestor->descendants.erase(it);
    }
  }

}  // namespace kagome::authority
