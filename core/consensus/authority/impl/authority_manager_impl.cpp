/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

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
#include "runtime/trie_storage_provider.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

using kagome::common::Buffer;
using kagome::consensus::grandpa::MembershipCounter;

namespace kagome::authority {

  AuthorityManagerImpl::AuthorityManagerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::TrieStorageProvider> trie_storage_provider,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      std::shared_ptr<crypto::Hasher> hasher)
      : block_tree_(std::move(block_tree)),
        trie_storage_provider_(std::move(trie_storage_provider)),
        grandpa_api_(std::move(grandpa_api)),
        hasher_(std::move(hasher)),
        log_{log::createLogger("AuthorityManager", "authority")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(trie_storage_provider_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    BOOST_ASSERT(app_state_manager != nullptr);
    app_state_manager->atPrepare([&] { return prepare(); });
  }

  bool AuthorityManagerImpl::prepare() {
    auto finalized_block_hash = block_tree_->getLastFinalized().hash;

    struct Args {
      primitives::ConsensusEngineId engine_id;
      primitives::BlockInfo block;
      primitives::Consensus message;
    };

    std::stack<Args> collected;

    {  // observe non-finalized blocks
      std::unordered_set<primitives::BlockHash> observed;
      for (auto &leaf : block_tree_->getLeaves()) {
        for (auto hash = leaf;;) {
          if (hash == finalized_block_hash) {
            break;
          }

          if (not observed.emplace(hash).second) {
            break;
          }

          auto header_res = block_tree_->getBlockHeader(hash);
          if (header_res.has_error()) {
            log_->critical("Can't get header of block {}: {}",
                           hash,
                           header_res.error().message());
            return false;
          }
          const auto &header = header_res.value();

          // observe possible changes of authorities
          for (auto &digest : header.digest) {
            visit_in_place(
                digest,
                [&](const primitives::Consensus &consensus_message) {
                  collected.emplace(
                      Args{consensus_message.consensus_engine_id,
                           primitives::BlockInfo(header.number, hash),
                           consensus_message});
                },
                [](const auto &) {});
          }

          hash = header.parent_hash;
        }
      }
    }

    {  // observe last finalized blocks
      bool found = false;
      for (auto hash = finalized_block_hash;;) {
        auto header_res = block_tree_->getBlockHeader(hash);
        if (header_res.has_error()) {
          log_->critical("Can't get header of block {}: {}",
                         hash,
                         header_res.error().message());
          return false;
        }
        const auto &header = header_res.value();

        // observe possible changes of authorities
        for (auto &digest : header.digest) {
          visit_in_place(
              digest,
              [&](const primitives::Consensus &consensus_message) {
                collected.emplace(
                    Args{consensus_message.consensus_engine_id,
                         primitives::BlockInfo(header.number, hash),
                         consensus_message});
                found = true;
              },
              [](const auto &...) {}); // Others variants is ignored
        }

        if (found || header.number == 0) {
          auto res =
              trie_storage_provider_->setToEphemeralAt(header.state_root);
          BOOST_ASSERT_MSG(
              res.has_value(),
              "Must be set ephemeral bath on existing state root; deq");
          auto batch = trie_storage_provider_->getCurrentBatch();

          std::optional<MembershipCounter> set_id_opt;
          for (auto prefix : {"GrandpaFinality", "Grandpa"}) {
            auto k1 = hasher_->twox_128(Buffer::fromString(prefix));
            auto k2 = hasher_->twox_128(Buffer::fromString("CurrentSetId"));
            auto set_id_key = Buffer().put(k1).put(k2);

            auto val_opt_res = batch->tryGet(set_id_key);
            if (val_opt_res.has_error()) {
              log_->critical("Can't get grandpa set id for block {}: {}",
                             primitives::BlockInfo(header.number, hash),
                             val_opt_res.error().message());
              return false;
            }
            auto &val_opt = val_opt_res.value();
            if (val_opt.has_value()) {
              auto &val = val_opt.value();
              set_id_opt.emplace(scale::decode<MembershipCounter>(val).value());
              break;
            }
          }

          if (not set_id_opt.has_value()) {
            log_->critical("Can't get grandpa set id for block {}: not found",
                           primitives::BlockInfo(header.number, hash));
            return false;
          }
          auto &set_id = set_id_opt.value();

          // Get initial authorities from genesis
          auto authorities_res = grandpa_api_->authorities(hash);
          if (not authorities_res.has_value()) {
            log_->critical("Can't get grandpa authorities for block {}: {}",
                           primitives::BlockInfo(header.number, hash),
                           authorities_res.error().message());
            return false;
          }
          auto &authorities = authorities_res.value();
          authorities.id = set_id;

          auto node =
              authority::ScheduleNode::createAsRoot({header.number, hash});
          node->actual_authorities =
              std::make_shared<primitives::AuthorityList>(
                  std::move(authorities));

          root_ = std::move(node);
          break;
        }

        hash = header.parent_hash;
      }
    }

    while (not collected.empty()) {
      const auto &args = collected.top();

      auto res = AuthorityManagerImpl::onConsensus(
          args.engine_id, args.block, args.message);
      if (res.has_error()) {
        log_->critical("Can't apply previous scheduled change: {}",
                       res.error().message());
        return false;
      }

      collected.pop();
    }

    SL_DEBUG(log_, "Authority set id: {:l}", root_->actual_authorities->id);
    for (const auto &authority : *root_->actual_authorities) {
      SL_DEBUG(log_, "Grandpa authority: {:l}", authority.id.id);
    }

    return true;
  }

  primitives::BlockInfo AuthorityManagerImpl::base() const {
    if (not root_) {
      log_->critical("Authority manager has null root");
      std::terminate();
    }
    return root_->block;
  }

  outcome::result<std::shared_ptr<const primitives::AuthorityList>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &block,
                                    bool finalized) {
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALISED;
    }

    auto adjusted_node = node->makeDescendant(block, finalized);

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
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

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
    for (auto &authority : *new_node->scheduled_authorities) {
      SL_VERBOSE(log_,
                 "New authority id={}, weight={}",
                 authority.id.id,
                 authority.weight);
    }

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;

      if (descendant->block.number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyForcedChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

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

    SL_VERBOSE(
        log_, "Change is forced on block #{}", new_node->scheduled_after);
    for (auto &authority : *new_node->forced_authorities) {
      SL_VERBOSE(log_,
                 "New authority id={}, weight={}",
                 authority.id.id,
                 authority.weight);
    }

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;

      // Apply forced changes if dalay will be passed for descendant
      if (descendant->block.number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }
      if (descendant->block.number >= ancestor->resume_for) {
        descendant->enabled = true;
        descendant->resume_for = ScheduleNode::INACTIVE;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyOnDisabled(
      const primitives::BlockInfo &block, uint64_t authority_index) {
    auto node = getAppropriateAncestor(block);

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
               "Authotity id={} is disabled on block #{}",
               (*authorities)[authority_index].id.id,
               new_node->block.number);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      if (directChainExists(block, descendant->block)) {
        // Propogate change to descendants
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

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->pause_after = activate_at;

    SL_VERBOSE(log_, "Scheduled pause after block #{}", new_node->block.number);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
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

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->resume_for = activate_at;

    SL_VERBOSE(log_, "Scheduled resume on block #{}", new_node->block.number);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;

      // Apply resume if delay will be passed for descendant
      if (descendant->block.number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }
      if (descendant->block.number >= ancestor->resume_for) {
        descendant->enabled = true;
        descendant->resume_for = ScheduleNode::INACTIVE;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onConsensus(
      const primitives::ConsensusEngineId &engine_id,
      const primitives::BlockInfo &block,
      const primitives::Consensus &message) {
    OUTCOME_TRY(message.decode());
    if (engine_id == primitives::kBabeEngineId) {
      // TODO(xDimon): Perhaps it needs to be refactored.
      //  It is better handle babe digests here
      //  Issue: https://github.com/soramitsu/kagome/issues/740
      return visit_in_place(
          message.asBabeDigest(),
          [](const primitives::NextEpochData &msg) -> outcome::result<void> {
            return outcome::success();
          },
          [](const primitives::OnDisabled &msg) {
            // Note: This event type wount be used anymore and must be ignored
            return outcome::success();
          },
          [](const primitives::NextConfigData &msg) {
            return outcome::success();
          },
          [](auto &) {
            return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
          });
    } else if (engine_id == primitives::kGrandpaEngineId) {
      return visit_in_place(
          message.asGrandpaDigest(),
          [this, &block](
              const primitives::ScheduledChange &msg) -> outcome::result<void> {
            return applyScheduledChange(
                block, msg.authorities, block.number + msg.subchain_length);
          },
          [this, &block](const primitives::ForcedChange &msg) {
            return applyForcedChange(
                block, msg.authorities, block.number + msg.subchain_length);
          },
          [](const primitives::OnDisabled &msg) {
            // Note: This event type wount be used anymore and must be ignored
            return outcome::success();
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
      return AuthorityManagerError::UNKNOWN_ENGINE_ID;
    }
  }

  outcome::result<void> AuthorityManagerImpl::prune(
      const primitives::BlockInfo &block) {
    if (block == root_->block) {
      return outcome::success();
    }

    if (block.number < root_->block.number) {
      return outcome::success();
    }

    auto node = getAppropriateAncestor(block);

    if (node->block == block) {
      // Rebase
      root_ = std::move(node);

    } else {
      // Reorganize ancestry
      auto new_node = node->makeDescendant(block, true);
      for (auto &descendant : std::move(node->descendants)) {
        if (directChainExists(block, descendant->block)) {
          new_node->descendants.emplace_back(std::move(descendant));
        }
      }

      root_ = std::move(new_node);
    }

    SL_VERBOSE(log_, "Prune authority manager upto block #{}", block.number);

    return outcome::success();
  }

  std::shared_ptr<ScheduleNode> AuthorityManagerImpl::getAppropriateAncestor(
      const primitives::BlockInfo &block) {
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
      const primitives::BlockInfo &descendant) {
    // Any block is descendant of genesis
    if (ancestor.number <= 1 && ancestor.number < descendant.number) {
      return true;
    }
    auto result =
        ancestor.number < descendant.number
        && block_tree_->hasDirectChain(ancestor.hash, descendant.hash);
    return result;
  }
}  // namespace kagome::authority
