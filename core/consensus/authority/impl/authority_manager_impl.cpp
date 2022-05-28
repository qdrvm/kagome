/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <stack>
#include <unordered_set>

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "consensus/authority/authority_manager_error.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"
#include "consensus/authority/impl/schedule_node.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/hasher.hpp"
#include "runtime/runtime_api/grandpa_api.hpp"
#include "scale/scale.hpp"
#include "storage/trie/trie_storage.hpp"

using kagome::common::Buffer;
using kagome::consensus::grandpa::MembershipCounter;

namespace kagome::authority {

  AuthorityManagerImpl::AuthorityManagerImpl(
      Config config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      std::shared_ptr<crypto::Hasher> hasher)
      : config_{std::move(config)},
        block_tree_(std::move(block_tree)),
        trie_storage_(std::move(trie_storage)),
        grandpa_api_(std::move(grandpa_api)),
        hasher_(std::move(hasher)),
        log_{log::createLogger("AuthorityManager", "authority")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(trie_storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

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
        for (auto &digest : header.digest) {
          visit_in_place(
              digest,
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
   * @param authorities - known authority set of the last finalized block
   * @param log - logger
   * @return new authority manager root node (or error)
   */
  outcome::result<std::shared_ptr<ScheduleNode>>
  collectConsensusMsgsUntilNearestSetChangeTo(
      std::stack<ConsensusMessages> &collected_msgs,
      primitives::BlockHash const &finalized_block_hash,
      blockchain::BlockTree const &block_tree,
      primitives::AuthorityList &authorities,
      log::Logger &log) {
    bool found_set_change = false;
    for (auto hash = finalized_block_hash; !found_set_change;) {
      OUTCOME_TRY(header, block_tree.getBlockHeader(hash));

      if (header.number == 0) {
        found_set_change = true;
      } else {
        // observe possible changes of authorities
        for (auto &digest : header.digest) {
          visit_in_place(
              digest,
              [&](const primitives::Consensus &consensus_message) {
                collected_msgs.emplace(ConsensusMessages{
                    primitives::BlockInfo(header.number, hash),
                    consensus_message});
                bool is_grandpa = consensus_message.consensus_engine_id
                                  == primitives::kGrandpaEngineId;
                auto decoded_res = consensus_message.decode();
                if (decoded_res.has_error()) {
                  log->critical("Error decoding consensus message: {}",
                                decoded_res.error().message());
                }
                auto &decoded = decoded_res.value();
                if (is_grandpa) {
                  bool is_scheduled_change =
                      decoded.isGrandpaDigestOf<primitives::ScheduledChange>();
                  bool is_forced_change =
                      decoded.isGrandpaDigestOf<primitives::ForcedChange>();
                  if (is_forced_change or is_scheduled_change) {
                    found_set_change = true;
                  }
                }
              },
              [](const auto &...) {});  // Other variants are ignored
        }
      }

      if (found_set_change) {
        SL_TRACE(
            log, "Found grandpa digest in block #{} ({})", header.number, hash);
        if (header.number != 0) {
          --authorities.id;
          SL_TRACE(log,
                   "Decrease authority ID to {}, as the found digest is an "
                   "authority set update",
                   authorities.id);
        }
        auto node =
            authority::ScheduleNode::createAsRoot({header.number, hash});
        node->actual_authorities =
            std::make_shared<primitives::AuthorityList>(std::move(authorities));

        return node;

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
    const auto &finalized_block_hash = finalized_block.hash;

    PREPARE_TRY(
        collected_msgs,
        collectMsgsFromNonFinalBlocks(*block_tree_, finalized_block_hash),
        "Error collecting consensus messages from non-finalized blocks: {}",
        error.message());

    primitives::AuthorityList authorities;
    {  // get voter set id at last finalized block
      const auto &hash = finalized_block_hash;
      PREPARE_TRY(header,
                  block_tree_->getBlockHeader(hash),
                  "Can't get header of block {}: {}",
                  hash,
                  error.message());

      auto &&set_id_opt_res =
          fetchSetIdFromTrieStorage(*trie_storage_, *hasher_, header);
      if (set_id_opt_res.has_error()) {
        auto &error = set_id_opt_res.error();
        log_->warn(
            "Couldn't fetch authority set id from trie storage for block #{} "
            "({}): {}. Recalculating from genesis.",
            header.number,
            hash,
            error.message());
        return prepareFromGenesis();
      }
      auto &set_id_opt = set_id_opt_res.value();

      if (not set_id_opt.has_value()) {
        log_->critical(
            "Can't get grandpa set id for block {}: "
            "CurrentSetId not found in Trie storage",
            primitives::BlockInfo(header.number, hash));
        return false;
      }
      const auto &set_id = set_id_opt.value();
      SL_TRACE(log_,
               "Initialized set id from runtime: #{} at block #{} ({})",
               set_id,
               header.number,
               hash);

      // Get initial authorities from runtime
      PREPARE_TRY(initial_authorities,
                  grandpa_api_->authorities(hash),
                  "Can't get grandpa authorities for block {}: {}",
                  primitives::BlockInfo(header.number, hash),
                  error.message());
      authorities = std::move(initial_authorities);
      authorities.id = set_id;
    }

    PREPARE_TRY(
        new_root,
        collectConsensusMsgsUntilNearestSetChangeTo(collected_msgs,
                                                    finalized_block_hash,
                                                    *block_tree_,
                                                    authorities,
                                                    log_),
        "Error collecting consensus messages from finalized blocks: {}",
        error.message());
    root_ = new_root;

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

    SL_DEBUG(log_, "Authority set id: {}", root_->actual_authorities->id);
    for (const auto &authority : *root_->actual_authorities) {
      SL_DEBUG(log_, "Grandpa authority: {}", authority.id.id);
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
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &block,
                                    bool finalized) const {
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return std::nullopt;
    }

    bool node_in_finalized_chain =
        not directChainExists(block_tree_->getLastFinalized(), node->block);

    auto adjusted_node = node->makeDescendant(block, node_in_finalized_chain);

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
             "Applying scheduled change for block {} to activate at block {}",
             block,
             activate_at);
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    SL_DEBUG(
        log_,
        "Oldest scheduled change before block {} is at block {} with set id {}",
        block,
        node->block,
        node->actual_authorities->id);
    auto last_finalized = block_tree_->getLastFinalized();
    bool node_in_finalized_chain =
        not directChainExists(last_finalized, node->block);

    SL_DEBUG(log_,
             "Last finalized is {}, is on the same chain as target block? {}",
             last_finalized,
             node_in_finalized_chain);

    auto new_node = node->makeDescendant(block, node_in_finalized_chain);
    SL_DEBUG(log_,
             "Make a schedule node for block {}, with actual set id {}",
             block,
             new_node->actual_authorities->id);

    auto res = new_node->ensureReadyToSchedule();
    if (!res) {
      SL_DEBUG(
          log_, "Node is not ready to be scheduled: {}", res.error().message());
      return res.as_failure();
    }

    auto new_authorities =
        std::make_shared<primitives::AuthorityList>(authorities);
    new_authorities->id = new_node->actual_authorities->id + 1;

    // Schedule change
    new_node->scheduled_authorities = std::move(new_authorities);
    new_node->scheduled_after = activate_at;

    SL_VERBOSE(log_,
               "Change is scheduled after block #{} (set id={})",
               new_node->scheduled_after,
               new_node->scheduled_authorities->id);
    size_t index = 0;
    for (auto &authority : *new_node->scheduled_authorities) {
      SL_VERBOSE(log_,
                 "New authority ({}/{}): id={} weight={}",
                 ++index,
                 new_node->scheduled_authorities->size(),
                 authority.id.id,
                 authority.weight);
    }

    // Reorganize ancestry
    reorganize(node, new_node);

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyForcedChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    auto new_authorities =
        std::make_shared<primitives::AuthorityList>(authorities);
    new_authorities->id = new_node->actual_authorities->id + 1;

    // Force changes
    if (new_node->block.number >= activate_at) {
      new_node->actual_authorities = std::move(new_authorities);
    } else {
      new_node->forced_authorities =
          std::make_shared<primitives::AuthorityList>(authorities);
      new_node->forced_for = activate_at;
    }

    SL_VERBOSE(log_, "Change is forced on block #{}", activate_at);
    size_t index = 0;
    for (auto &authority : *new_node->forced_authorities) {
      SL_VERBOSE(log_,
                 "New authority ({}/{}): id={} weight={}",
                 ++index,
                 new_node->forced_authorities->size(),
                 authority.id.id,
                 authority.weight);
    }

    // Reorganize ancestry
    reorganize(node, new_node);

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyOnDisabled(
      const primitives::BlockInfo &block, uint64_t authority_index) {
    if (!config_.on_disable_enabled) {
      SL_TRACE(log_, "Ignore 'on disabled' message due to config");
      return outcome::success();
    }
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    auto new_node = node->makeDescendant(block);

    // Check if index not out of bound
    if (authority_index >= node->actual_authorities->size()) {
      return AuthorityUpdateObserverError::WRONG_AUTHORITY_INDEX;
    }

    // Make changed authorities
    auto authorities = std::make_shared<primitives::AuthorityList>(
        *new_node->actual_authorities);
    (*authorities)[authority_index].weight = 0;
    new_node->actual_authorities = std::move(authorities);

    SL_VERBOSE(log_,
               "Authority id={} is disabled on block #{}",
               (*authorities)[authority_index].id.id,
               new_node->block.number);

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

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyPause(
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALIZED;
    }

    bool node_in_finalized_chain =
        not directChainExists(block_tree_->getLastFinalized(), node->block);

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

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->resume_for = activate_at;

    SL_VERBOSE(log_, "Scheduled resume on block #{}", new_node->block.number);

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
      return visit_in_place(
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
    } else {
      // SL_WARN(log_,
      //         "Unknown consensus engine id in block {}: {}",
      //         block,
      //         message.consensus_engine_id.toString());
      return outcome::success();
    }
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

  bool AuthorityManagerImpl::prepareFromGenesis() {
    auto t_start = std::chrono::high_resolution_clock::now();
    std::vector<std::pair<primitives::BlockHash, primitives::BlockHeader>>
        headers;
    const auto finalized_block = block_tree_->getLastFinalized();
    auto header = block_tree_->getBlockHeader(finalized_block.hash);
    headers.emplace_back(finalized_block.hash, header.value());
    size_t count1 = 0;
    while (header.has_value()) {
      auto hash = header.value().parent_hash;
      header = block_tree_->getBlockHeader(hash);
      if (header.has_value()) {
        if (not(++count1 % 10000)) {
          SL_WARN(log_, "{} headers loaded", count1);
        }
        headers.emplace_back(hash, header.value());
      }
    }

    root_ = authority::ScheduleNode::createAsRoot(
        {headers.back().second.number, headers.back().first});
    auto authorities = grandpa_api_->authorities(headers.back().first).value();
    authorities.id = 0;
    root_->actual_authorities =
        std::make_shared<primitives::AuthorityList>(std::move(authorities));

    count1 = 0;
    size_t count2 = 0;
    for (const auto &[hash, header] : boost::range::reverse(headers)) {
      if (not(++count1 % 1000)) {
        SL_WARN(log_, "{} digests applied ({})", count1, count2);
        count2 = 0;
      }
      for (const auto &digest_item : header.digest) {
        std::ignore = visit_in_place(
            digest_item,
            [&](const primitives::Consensus &consensus_message)
                -> outcome::result<void> {
              ++count2;
              return onConsensus(primitives::BlockInfo{header.number, hash},
                                 consensus_message);
            },
            [](const auto &) { return outcome::success(); });
      }
      if (not(count1 % 10000)) {
        prune(primitives::BlockInfo{header.number, hash});
      }
    }

    auto t_end = std::chrono::high_resolution_clock::now();

    log_->warn(
        "Applied authorities within {} ms",
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start)
            .count());
    return true;
  }

}  // namespace kagome::authority
