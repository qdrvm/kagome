/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

#include <stack>
#include <unordered_set>

#include <boost/range/adaptor/reversed.hpp>

#include "application/app_state_manager.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "consensus/authority/authority_manager_error.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"
#include "consensus/authority/impl/schedule_node.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/hasher.hpp"
#include "runtime/runtime_api/grandpa_api.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/trie_storage.hpp"

using kagome::common::Buffer;
using kagome::consensus::grandpa::MembershipCounter;

namespace kagome::authority {

  AuthorityManagerImpl::AuthorityManagerImpl(
      Config config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<storage::BufferStorage> buffer_storage)
      : config_{std::move(config)},
        header_repo_(std::move(header_repo)),
        block_tree_(std::move(block_tree)),
        trie_storage_(std::move(trie_storage)),
        grandpa_api_(std::move(grandpa_api)),
        hasher_(std::move(hasher)),
        buffer_storage_(std::move(buffer_storage)),
        log_{log::createLogger("AuthorityManager", "authority")} {
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(trie_storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(buffer_storage_ != nullptr);

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

        OUTCOME_TRY(header, block_tree.getBlockHeader(hash));

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

  outcome::result<std::optional<MembershipCounter>> fetchSetIdFromTrieStorage(
      storage::trie::TrieStorage const &trie_storage,
      crypto::Hasher const &hasher,
      primitives::BlockHeader const &header) {
    OUTCOME_TRY(batch, trie_storage.getEphemeralBatchAt(header.state_root));

    std::optional<MembershipCounter> set_id_opt;
    auto current_set_id_keypart =
        hasher.twox_128(Buffer::fromString("CurrentSetId"));
    for (auto prefix : {"GrandpaFinality", "Grandpa"}) {
      auto prefix_key_part = hasher.twox_128(Buffer::fromString(prefix));
      auto set_id_key =
          Buffer().put(prefix_key_part).put(current_set_id_keypart);

      OUTCOME_TRY(val_opt, batch->tryGet(set_id_key));
      if (val_opt.has_value()) {
        auto &val = val_opt.value();
        set_id_opt.emplace(scale::decode<MembershipCounter>(val.get()).value());
        break;
      }
    }
    return set_id_opt;
  }

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
      OUTCOME_TRY(header, block_tree.getBlockHeader(hash));

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
    if (load()) {
      return true;
    }

    const auto finalized_block = block_tree_->getLastFinalized();

    PREPARE_TRY(
        collected_msgs,
        collectMsgsFromNonFinalBlocks(*block_tree_, finalized_block.hash),
        "Error collecting consensus messages from non-finalized blocks: {}",
        error.message());

    PREPARE_TRY(significant_block,
                collectConsensusMsgsUntilNearestSetChangeTo(
                    collected_msgs, finalized_block, *block_tree_, log_),
                "Error collecting consensus messages from finalized blocks: {}",
                error.message());

    primitives::AuthorityList authorities;
    {  // get voter set id at earliest significant block
      const auto &hash = significant_block.hash;
      PREPARE_TRY(header,
                  block_tree_->getBlockHeader(hash),
                  "Can't get header of block {}: {}",
                  significant_block,
                  error.message());

      auto &&set_id_opt_res =
          fetchSetIdFromTrieStorage(*trie_storage_, *hasher_, header);
      if (set_id_opt_res.has_error()) {
        log_->warn(
            "Can't fetch authority set id from trie storage for block {}: {}",
            primitives::BlockInfo(header.number, hash),
            set_id_opt_res.error().message());
        log_->info(
            "Recalculating from genesis "
            "(going to take a few dozens of seconds)");
        return prepareFromGenesis();
      }
      auto &set_id_opt = set_id_opt_res.value();

      if (not set_id_opt.has_value()) {
        log_->critical(
            "Can't get grandpa set id for block {}: "
            "CurrentSetId not found in Trie storage",
            significant_block);
        return false;
      }
      const auto &set_id = set_id_opt.value();
      SL_TRACE(log_,
               "Initialized set id from runtime: #{} at block {}",
               set_id,
               significant_block);

      // Get initial authorities from runtime
      PREPARE_TRY(initial_authorities,
                  grandpa_api_->authorities(hash),
                  "Can't get grandpa authorities for block {}: {}",
                  significant_block,
                  error.message());
      authorities = std::move(initial_authorities);
      authorities.id = set_id;
    }

    auto node = authority::ScheduleNode::createAsRoot(significant_block);

    node->actual_authorities =
        std::make_shared<primitives::AuthorityList>(std::move(authorities));

    root_ = std::move(node);

    while (not collected_msgs.empty()) {
      const auto &args = collected_msgs.top();
      SL_TRACE(log_,
               "Apply consensus message from block {}, engine {}",
               args.block,
               args.message.consensus_engine_id.toString());
      PREPARE_TRY_VOID(onConsensus(args.block, args.message),
                       "Can't apply previous consensus message: {}",
                       error.message());

      collected_msgs.pop();
    }

    // prune to reorganize collected changes
    prune(finalized_block);

    SL_DEBUG(log_,
             "Actual grandpa authority set (id={}):",
             root_->actual_authorities->id);
    size_t index = 0;
    for (const auto &authority : *root_->actual_authorities) {
      SL_DEBUG(log_,
               "{}/{}: id={} weight={}",
               ++index,
               root_->actual_authorities->size(),
               authority.id.id,
               authority.weight);
    }

    return true;
  }

#undef PREPARE_TRY_VOID
#undef PREPARE_TRY

  primitives::BlockInfo AuthorityManagerImpl::base() const {
    if (not root_) {
      log_->critical("Authority manager has null root");
      std::terminate();
    }
    return root_->block;
  }

  std::optional<std::shared_ptr<const primitives::AuthorityList>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &target_block,
                                    IsBlockFinalized finalized) const {
    auto node = getAppropriateAncestor(target_block);

    if (not node) {
      return std::nullopt;
    }

    IsBlockFinalized node_in_finalized_chain =
        node->block == target_block
            ? (bool)finalized
            : (node->block == block_tree_->getLastFinalized()
               or directChainExists(node->block,
                                    block_tree_->getLastFinalized()));

    auto adjusted_node =
        node->makeDescendant(target_block, node_in_finalized_chain);

    if (adjusted_node->enabled) {
      // Original authorities
      return adjusted_node->actual_authorities;
    }

    // Zero-weighted authorities
    auto authorities = std::make_shared<primitives::AuthorityList>(
        *adjusted_node->actual_authorities);
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
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    SL_DEBUG(log_,
             "Actual authorities for block {} found on block {} with set id {}",
             block,
             node->block,
             node->actual_authorities->id);

    auto schedule_change = [&](const std::shared_ptr<ScheduleNode> &node)
        -> outcome::result<void> {
      auto res = node->ensureReadyToSchedule();
      if (!res) {
        SL_DEBUG(log_,
                 "Node is not ready to schedule scheduled change: {}",
                 res.error().message());
        return res.as_failure();
      }

      auto new_authorities =
          std::make_shared<primitives::AuthorityList>(authorities);
      new_authorities->id = node->actual_authorities->id + 1;

      // Schedule change
      node->scheduled_authorities = std::move(new_authorities);
      node->scheduled_after = activate_at;

      SL_VERBOSE(log_,
                 "Change is scheduled after block #{} (set id={})",
                 node->scheduled_after,
                 node->scheduled_authorities->id);

      size_t index = 0;
      for (auto &authority : *node->scheduled_authorities) {
        SL_VERBOSE(log_,
                   "New authority ({}/{}): id={} weight={}",
                   ++index,
                   node->scheduled_authorities->size(),
                   authority.id.id,
                   authority.weight);
      }

      return outcome::success();
    };

    IsBlockFinalized node_in_finalized_chain =
        node->block == block_tree_->getLastFinalized()
        or directChainExists(node->block, block_tree_->getLastFinalized());

    if (node->block == block) {
      node->adjust(node_in_finalized_chain);
      OUTCOME_TRY(schedule_change(node));
    } else {
      auto new_node = node->makeDescendant(block, node_in_finalized_chain);
      SL_DEBUG(log_,
               "Make a schedule node for block {}, with actual set id {}",
               block,
               new_node->actual_authorities->id);

      OUTCOME_TRY(schedule_change(new_node));

      // Reorganize ancestry
      reorganize(node, new_node);
    }

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyForcedChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    SL_DEBUG(log_,
             "Applying forced change on block {} to activate at block {}",
             block,
             activate_at);
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    auto force_change = [&](const std::shared_ptr<ScheduleNode> &node)
        -> outcome::result<void> {
      auto res = node->ensureReadyToSchedule();
      if (!res) {
        SL_DEBUG(log_,
                 "Node is not ready to schedule forced change: {}",
                 res.error().message());
        return res.as_failure();
      }

      auto new_authorities =
          std::make_shared<primitives::AuthorityList>(authorities);
      new_authorities->id = node->actual_authorities->id + 1;

      // Force changes
      if (node->block.number >= activate_at) {
        node->actual_authorities = std::move(new_authorities);
      } else {
        node->forced_authorities =
            std::make_shared<primitives::AuthorityList>(authorities);
        node->forced_for = activate_at;
      }

      SL_VERBOSE(log_,
                 "Change will be forced on block #{} (set id={})",
                 activate_at,
                 node->forced_authorities->id);
      size_t index = 0;
      for (auto &authority : *node->forced_authorities) {
        SL_VERBOSE(log_,
                   "New authority ({}/{}): id={} weight={}",
                   ++index,
                   node->forced_authorities->size(),
                   authority.id.id,
                   authority.weight);
      }

      return outcome::success();
    };

    IsBlockFinalized node_in_finalized_chain =
        node->block == block_tree_->getLastFinalized()
        or directChainExists(node->block, block_tree_->getLastFinalized());

    if (node->block == block) {
      node->adjust(node_in_finalized_chain);
      OUTCOME_TRY(force_change(node));
    } else {
      auto new_node = node->makeDescendant(block, node_in_finalized_chain);

      OUTCOME_TRY(force_change(new_node));

      // Reorganize ancestry
      reorganize(node, new_node);
    }

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
      auto authorities = std::make_shared<primitives::AuthorityList>(
          *node->actual_authorities);

      // Check if index not out of bound
      if (authority_index >= node->actual_authorities->size()) {
        return AuthorityUpdateObserverError::WRONG_AUTHORITY_INDEX;
      }

      (*authorities)[authority_index].weight = 0;
      node->actual_authorities = std::move(authorities);

      SL_VERBOSE(
          log_,
          "Authority id={} (index={} in set id={}) is disabled on block #{}",
          (*node->actual_authorities)[authority_index].id.id,
          authority_index,
          node->actual_authorities->id,
          node->block.number);

      return outcome::success();
    };

    IsBlockFinalized node_in_finalized_chain =
        node->block == block_tree_->getLastFinalized()
        or directChainExists(node->block, block_tree_->getLastFinalized());

    if (node->block == block) {
      node->adjust(node_in_finalized_chain);
      OUTCOME_TRY(disable_authority(node));
    } else {
      auto new_node = node->makeDescendant(block, node_in_finalized_chain);

      OUTCOME_TRY(disable_authority(new_node));

      // Reorganize ancestry
      auto descendants = std::move(node->descendants);
      for (auto &descendant : descendants) {
        if (directChainExists(block, descendant->block)) {
          // Propagate change to descendants
          if (descendant->actual_authorities == node->actual_authorities) {
            descendant->actual_authorities = new_node->actual_authorities;
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
        node->block == block_tree_->getLastFinalized()
        or directChainExists(node->block, block_tree_->getLastFinalized());

    auto new_node = node->makeDescendant(block, node_in_finalized_chain);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->pause_after = activate_at;

    SL_VERBOSE(log_, "Scheduled pause after block #{}", new_node->block.number);

    // Reorganize ancestry
    auto descendants = std::move(node->descendants);
    for (auto &descendant : descendants) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;
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
        node->block == block_tree_->getLastFinalized()
        or directChainExists(node->block, block_tree_->getLastFinalized());

    auto new_node = node->makeDescendant(block, node_in_finalized_chain);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->resume_for = activate_at;

    SL_VERBOSE(
        log_, "Resuming will be done at block #{}", new_node->block.number);

    // Reorganize ancestry
    reorganize(node, new_node);

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onConsensus(
      const primitives::BlockInfo &block,
      const primitives::Consensus &message) {
    if (message.consensus_engine_id == primitives::kBabeEngineId) {
      OUTCOME_TRY(decoded, message.decode());
      // TODO(xDimon): Perhaps it needs to be refactored.
      //  It is better handle babe digests here
      //  Issue: https://github.com/soramitsu/kagome/issues/740
      return visit_in_place(
          decoded.asBabeDigest(),
          [](const primitives::NextEpochData &msg) -> outcome::result<void> {
            return outcome::success();
          },
          [](const primitives::OnDisabled &msg) {
            // Note: This event type won't be used anymore and must be ignored
            return outcome::success();
          },
          [](const primitives::NextConfigData &msg) {
            return outcome::success();
          },
          [](auto &) {
            return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
          });
    }
    if (message.consensus_engine_id == primitives::kGrandpaEngineId) {
      OUTCOME_TRY(decoded, message.decode());
      auto res = visit_in_place(
          decoded.asGrandpaDigest(),
          [this, &block](
              const primitives::ScheduledChange &msg) -> outcome::result<void> {
            return applyScheduledChange(
                block, msg.authorities, block.number + msg.subchain_length);
          },
          [this, &block](const primitives::ForcedChange &msg) {
            return applyForcedChange(
                block, msg.authorities, block.number + msg.subchain_length);
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
      save();
      return res;
    } else if (message.consensus_engine_id
                   == primitives::kUnsupportedEngineId_POL1
               or message.consensus_engine_id
                      == primitives::kUnsupportedEngineId_BEEF) {
      SL_DEBUG(log_,
               "Unsupported consensus engine id in block {}: {}",
               block,
               message.consensus_engine_id.toString());
      return outcome::success();

    } else {
      SL_WARN(log_,
              "Unknown consensus engine id in block {}: {}",
              block,
              message.consensus_engine_id.toString());
      return outcome::success();
    }
  }

  void AuthorityManagerImpl::save() {
    auto data = scale::encode(root_).value();
    buffer_storage_->put(storage::kAuthorityManagerStateLookupKey, Buffer(data))
        .value();
  }

  bool AuthorityManagerImpl::load() {
    auto r = buffer_storage_->tryLoad(storage::kAuthorityManagerStateLookupKey)
                 .value();
    if (r.has_value()) {
      root_ = scale::decode<std::shared_ptr<ScheduleNode>>(r.value()).value();

      const auto finalized_block = block_tree_->getLastFinalized();

      auto node = getAppropriateAncestor(finalized_block);

      if (not node or node->block.number + 512 < finalized_block.number) {
        SL_WARN(log_,
                "Cached state of authority manager will be rejected: "
                "it does not math last finalized block");
        root_.reset();
        return false;
      }

      SL_DEBUG(log_, "State of authority manager is is restored from cache");
      return true;
    }

    SL_TRACE(log_, "Cached state of authority manager is not found");
    return false;
  }

  void AuthorityManagerImpl::prune(const primitives::BlockInfo &block) {
    if (block == root_->block) {
      return;
    }

    if (block.number < root_->block.number) {
      return;
    }

    auto node = getAppropriateAncestor(block);

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

    SL_VERBOSE(log_, "Prune authority manager upto block {}", block);

    save();
  }

  std::shared_ptr<ScheduleNode> AuthorityManagerImpl::getAppropriateAncestor(
      const primitives::BlockInfo &block) const {
    BOOST_ASSERT(root_ != nullptr);
    std::shared_ptr<ScheduleNode> ancestor;
    // Target block is not descendant of the current root
    if (root_->block.number > block.number
        || (root_->block != block
            && not directChainExists(root_->block, block))) {
      return ancestor;
    }
    ancestor = root_;
    while (ancestor->block != block) {
      bool goto_next_generation = false;
      for (const auto &node : ancestor->descendants) {
        if (node->block == block || directChainExists(node->block, block)) {
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
    // Any block is descendant of genesis
    if (ancestor.number <= 1 && ancestor.number < descendant.number) {
      return true;
    }
    auto result =
        ancestor.number < descendant.number
        && block_tree_->hasDirectChain(ancestor.hash, descendant.hash);
    return result;
  }

  void AuthorityManagerImpl::reorganize(
      std::shared_ptr<ScheduleNode> node,
      std::shared_ptr<ScheduleNode> new_node) {
    auto descendants = std::move(node->descendants);
    for (auto &descendant : descendants) {
      auto &ancestor = directChainExists(new_node->block, descendant->block)
                           ? new_node
                           : node;

      // Apply if delay will be passed for descendant
      if (ancestor->forced_for != ScheduleNode::INACTIVE) {
        if (descendant->block.number >= ancestor->forced_for) {
          descendant->actual_authorities = ancestor->forced_authorities;
          descendant->forced_authorities.reset();
          descendant->forced_for = ScheduleNode::INACTIVE;
        }
      }
      if (ancestor->resume_for != ScheduleNode::INACTIVE) {
        if (descendant->block.number >= ancestor->resume_for) {
          descendant->enabled = true;
          descendant->resume_for = ScheduleNode::INACTIVE;
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

    if (ancestor->block == block) {
      ancestor = std::const_pointer_cast<ScheduleNode>(ancestor->parent.lock());
    }

    auto it = std::find_if(ancestor->descendants.begin(),
                           ancestor->descendants.end(),
                           [&block](std::shared_ptr<ScheduleNode> node) {
                             return node->block == block;
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

    save();
  }

  bool AuthorityManagerImpl::prepareFromGenesis() {
    auto t_start = std::chrono::high_resolution_clock::now();

    const auto finalized_block = block_tree_->getLastFinalized();

    root_ = authority::ScheduleNode::createAsRoot(
        {0, block_tree_->getGenesisBlockHash()});

    auto authorities =
        grandpa_api_->authorities(block_tree_->getGenesisBlockHash()).value();
    authorities.id = 0;
    root_->actual_authorities =
        std::make_shared<primitives::AuthorityList>(std::move(authorities));

    size_t count = 0;
    size_t delta = 0;

    for (consensus::grandpa::BlockNumber block_number = 1;
         block_number <= finalized_block.number;
         ++block_number) {
      auto hash_res = header_repo_->getHashByNumber(block_number);
      if (hash_res.has_error()) {
        SL_CRITICAL(log_,
                    "Can't get hash of block #{} of the best chain: {}",
                    block_number,
                    hash_res.error().message());
        return false;
      }
      const auto &block_hash = hash_res.value();

      primitives::BlockInfo block_info(block_number, block_hash);

      auto header_res = block_tree_->getBlockHeader(block_hash);
      if (header_res.has_error()) {
        SL_CRITICAL(log_,
                    "Can't get header of block {}: {}",
                    block_info,
                    header_res.error().message());
        return false;
      }
      const auto &header = header_res.value();

      bool found = false;
      for (const auto &digest_item : header.digest) {
        std::ignore = visit_in_place(
            digest_item,
            [&](const primitives::Consensus &msg) -> outcome::result<void> {
              if (msg.consensus_engine_id == primitives::kGrandpaEngineId) {
                ++count;
                ++delta;
                found = true;
              }
              return onConsensus(block_info, msg);
            },
            [](const auto &) { return outcome::success(); });
      }
      if (not(block_number % 512)) {
        prune(block_info);
      }

      if (not(block_number % 250000))
      //      if (found)
      {
        SL_INFO(log_,
                "{} headers are observed and {} digests are applied (+{})",
                block_number,
                count,
                delta);
        delta = 0;
      }
    }

    auto t_end = std::chrono::high_resolution_clock::now();

    SL_INFO(log_,
            "{} headers are observed and {} digests are applied within {}s",
            finalized_block.number,
            count,
            std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start)
                .count());

    return true;
  }

}  // namespace kagome::authority
