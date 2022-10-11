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
#include "blockchain/block_tree_error.hpp"
#include "common/visitor.hpp"
#include "consensus/authority/authority_manager_error.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"
#include "consensus/authority/impl/schedule_node.hpp"
#include "crypto/hasher.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/runtime_api/grandpa_api.hpp"
#include "storage/database_error.hpp"
#include "storage/trie/trie_storage.hpp"

using kagome::common::Buffer;
using kagome::primitives::AuthoritySetId;

namespace kagome::authority {

  struct ScheduledChangeEntry {
    primitives::AuthoritySet new_authorities;
    primitives::BlockInfo scheduled_at;
    primitives::BlockNumber activates_at;
  };

  template <typename T>
  struct ForkTree {
    T payload;
    primitives::BlockInfo block;
    std::vector<std::unique_ptr<ForkTree>> children;
    ForkTree *parent;

    ForkTree *find(primitives::BlockHash const &block_hash) const {
      if (block.hash == block_hash) return this;
      for (auto &child : children) {
        if (auto *res = child->find(block_hash); res != nullptr) {
          return res;
        }
      }
      return nullptr;
    }

    bool forEachLeaf(std::function<bool(ForkTree &)> const &f) {
      if (children.empty()) {
        return f(*this);
      }
      for (auto &child : children) {
        bool res = child->forEachLeaf(f);
        if (res) return true;
      }
      return false;
    }

    std::unique_ptr<ForkTree<T>> detachChild(ForkTree<T> &child) {
      for (size_t i = 0; i < children.size(); i++) {
        auto &current_child = children[i];
        if (current_child.get() == &child) {
          auto owned_child = std::move(current_child);
          owned_child->parent = nullptr;
          children.erase(children.begin() + i);
          return owned_child;
        }
      }
      return nullptr;
    }
  };

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const ScheduledChangeEntry &entry) {
    s << entry.scheduled_at << entry.activates_at << entry.new_authorities;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          ScheduledChangeEntry &entry) {
    s >> entry.scheduled_at >> entry.activates_at >> entry.new_authorities;
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const ScheduleTree &tree) {
    s << tree.block << tree.payload << tree.children;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          ScheduleTree &change) {
    s >> change.block >> change.payload >> change.children;
    for (auto &child : change.children) {
      child->parent = &change;
    }
    return s;
  }

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
      storage::trie::TrieBatch const &trie_batch,
      crypto::Hasher const &hasher,
      storage::trie::RootHash const &state) {
    std::optional<AuthoritySetId> set_id_opt;
    auto current_set_id_keypart =
        hasher.twox_128(Buffer::fromString("CurrentSetId"));
    for (auto prefix : {"GrandpaFinality", "Grandpa"}) {
      auto prefix_key_part = hasher.twox_128(Buffer::fromString(prefix));
      auto set_id_key =
          Buffer().put(prefix_key_part).put(current_set_id_keypart);

      OUTCOME_TRY(val_opt, trie_batch.tryGet(set_id_key));
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

  static const Buffer storage_key =
      Buffer::fromString("test_authority_manager_root");

  AuthorityManagerImpl::~AuthorityManagerImpl() {}

  bool AuthorityManagerImpl::prepare() {
    const auto finalized_block = block_tree_->getLastFinalized();
    auto res = initializeAt(finalized_block);
    if (!res) {
      SL_ERROR(log_,
               "Error initializing authority manager: {}",
               res.error().message());
    }
    return res.has_value();
  }

  outcome::result<void> AuthorityManagerImpl::initializeAt(
      const primitives::BlockInfo &root_block) {
    OUTCOME_TRY(collected_msgs,
                collectMsgsFromNonFinalBlocks(*block_tree_, root_block.hash));

    OUTCOME_TRY(graph_root_block,
                collectConsensusMsgsUntilNearestSetChangeTo(
                    collected_msgs, root_block, *block_tree_, log_));

    OUTCOME_TRY(root_header,
                block_tree_->getBlockHeader(graph_root_block.hash));

    auto set_id_from_runtime_res = readSetIdFromRuntime(root_header);

    OUTCOME_TRY(opt_root, fetchScheduleGraphRoot(*persistent_storage_));
    auto last_finalized_block = block_tree_->getLastFinalized();

    if (opt_root
        && opt_root.value()->current_block.number
               <= last_finalized_block.number) {
      // TODO(Harrm): #1334
      // Correction to bypass the bug where after finishing syncing
      // and restarting the node we get a set id off by one
      if (set_id_from_runtime_res.has_value()
          && opt_root.value()->current_authorities->id
                 == set_id_from_runtime_res.value() - 1) {
        auto &authority_list =
            opt_root.value()->current_authorities->authorities;
        opt_root.value()->current_authorities =
            std::make_shared<primitives::AuthoritySet>(
                set_id_from_runtime_res.value(), authority_list);
      }

      root_ = std::move(opt_root.value());
      SL_DEBUG(log_,
               "Fetched authority set graph root from database with id {}",
               root_->current_authorities->id);

    } else if (last_finalized_block.number == 0) {
      auto &genesis_hash = block_tree_->getGenesisBlockHash();
      OUTCOME_TRY(initial_authorities, grandpa_api_->authorities(genesis_hash));
      root_ = authority::ScheduleNode::createAsRoot(
          std::make_shared<primitives::AuthoritySet>(
              0, std::move(initial_authorities)),
          {0, genesis_hash});
    } else if (set_id_from_runtime_res.has_value()){
      SL_WARN(
          log_,
          "Storage does not contain valid info about the root authority set; "
          "Fall back to obtaining it from the runtime storage (which may "
          "fail after a forced authority change happened on chain)");
      OUTCOME_TRY(authorities,
                  grandpa_api_->authorities(graph_root_block.hash));

      auto authority_set = std::make_shared<primitives::AuthoritySet>(
          set_id_from_runtime_res.value(), std::move(authorities));
      root_ = authority::ScheduleNode::createAsRoot(authority_set,
                                                    graph_root_block);

      OUTCOME_TRY(storeScheduleGraphRoot(*persistent_storage_, *root_));
      SL_TRACE(log_,
               "Create authority set graph root with id {}, taken from runtime "
               "storage",
               root_->current_authorities->id);
    } else {
      SL_ERROR(log_, "Failed to initialize authority manager; Try running recovery mode");
      return set_id_from_runtime_res.as_failure();
    }

    while (not collected_msgs.empty()) {
      const auto &args = collected_msgs.top();
      OUTCOME_TRY(onConsensus(args.block, args.message));

      collected_msgs.pop();
    }

    // prune to reorganize collected changes
    prune(root_block);

    SL_DEBUG(log_,
             "Current grandpa authority set (id={}):",
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

    return outcome::success();
  }

  outcome::result<std::optional<AuthoritySetId>>
  AuthorityManagerImpl::readSetIdFromRuntime(
      primitives::BlockHeader const &header) const {
    AuthoritySetId set_id{};
    OUTCOME_TRY(hash, primitives::calculateBlockHash(header, *hasher_));
    auto set_id_res = grandpa_api_->current_set_id(hash);
    if (set_id_res) {
      set_id = set_id_res.value();
    } else {
      auto batch_res = trie_storage_->getEphemeralBatchAt(header.state_root);
      if (batch_res.has_error()) {
        if (batch_res.error() == storage::DatabaseError::NOT_FOUND) {
          SL_DEBUG(
              log_,
              "Failed to fetch set id from trie storage: state {} is not in "
              "the storage",
              header.state_root);
          return std::nullopt;
        }
        return batch_res.as_failure();
      }
      auto &batch = batch_res.value();

      OUTCOME_TRY(
          set_id_opt,
          fetchSetIdFromTrieStorage(*batch, *hasher_, header.state_root));
      if (set_id_opt) return set_id_opt.value();

      SL_DEBUG(log_,
               "Failed to read authority set id from runtime (attempted both "
               "GrandpaApi_current_set_id and trie storage)");
      return std::nullopt;
    }
    return set_id;
  }

  outcome::result<void> AuthorityManagerImpl::recalculateStoredState(
      primitives::BlockNumber last_finalized_number) {
    auto genesis_hash = block_tree_->getGenesisBlockHash();

    OUTCOME_TRY(initial_authorities, grandpa_api_->authorities(genesis_hash));
    primitives::BlockInfo genesis_info{0, block_tree_->getGenesisBlockHash()};

    root_ = ScheduleNode::createAsRoot(
        std::make_shared<primitives::AuthoritySet>(0, initial_authorities),
        {0, genesis_hash});
    SL_INFO(log_,
            "Recovering authority manager state... (might take a few minutes)");
    // if state is pruned
    if (header_repo_->getBlockHeader(1).has_error()) {
      SL_WARN(log_,
              "Can't recalculate authority set id on a prune state, fall"
              " back to fetching from runtime");
      return clearScheduleGraphRoot(*persistent_storage_);
    }

    auto start = std::chrono::steady_clock::now();
    for (primitives::BlockNumber number = 0; number <= last_finalized_number;
         number++) {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(number));
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
      auto duration = end - start;
      using namespace std::chrono_literals;
      // 5 seconds is nothing special, just a random more-or-like convenient
      // duration.
      if (duration > 5s) {
        SL_VERBOSE(log_,
                   "Processed {} out of {} blocks",
                   number,
                   last_finalized_number);
        start = end;
      }
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

    auto adjusted_node =
        node->makeDescendant(target_block, node_in_finalized_chain);

    if (adjusted_node->enabled) {
      // Original authorities
      SL_DEBUG(log_,
               "Pick authority set with id {} for block {}",
               adjusted_node->current_authorities->id,
               target_block);
      for (auto &authority : adjusted_node->current_authorities->authorities) {
        SL_TRACE(log_, "Authority {}: {}", authority.id.id, authority.weight);
      }
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

      SL_VERBOSE(
          log_,
          "Authority set change is scheduled after block #{} (set id={})",
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

    KAGOME_PROFILE_START(is_ancestor_node_finalized)
    IsBlockFinalized is_ancestor_node_finalized =
        ancestor_node->current_block == block_tree_->getLastFinalized()
        or directChainExists(ancestor_node->current_block,
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
          if (block.number <= action->applied_block) {
            // It's mean, that new Scheduled Changes would be scheduled before
            // previous is activated. So we ignore it
            return outcome::success();
          }

          if (action->new_authorities->id
              > ancestor_node->current_authorities->id) {
            maybe_set = action->new_authorities;
          }
          break;
        }

        last_node = last_node->parent.lock();
      }
    }

    if (ancestor_node->current_block == block) {
      if (maybe_set.has_value()) {
        ancestor_node->current_authorities = maybe_set.value();
      } else {
        ancestor_node->adjust(is_ancestor_node_finalized);
      }

      OUTCOME_TRY(schedule_change(ancestor_node));
    } else {
      KAGOME_PROFILE_START(make_descendant)
      auto new_node = ancestor_node->makeDescendant(block, true);
      KAGOME_PROFILE_END(make_descendant)

      if (maybe_set.has_value()) {
        new_node->current_authorities = maybe_set.value();
      }

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
    auto delay_start_hash_res = header_repo_->getHashByNumber(delay_start);
    if (delay_start_hash_res.has_error()) {
      SL_ERROR(log_, "Failed to obtain hash by number {}", delay_start);
    }
    OUTCOME_TRY(delay_start_hash, delay_start_hash_res);
    auto ancestor_node =
        getAppropriateAncestor({delay_start, delay_start_hash});

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
        {delay_start, delay_start_hash}, true);

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
            throw std::runtime_error{"not implemented"};
            return applyForcedChange(
                block, msg.authorities, msg.delay_start, msg.subchain_length);
          },
          [this, &block](const primitives::OnDisabled &msg) {
            SL_DEBUG(log_, "OnDisabled {}", msg.authority_index);
            return applyOnDisabled(block, msg.authority_index);
          },
          [this, &block](const primitives::Pause &msg) {
            SL_DEBUG(log_, "Pause {}", msg.subchain_length);
            return applyPause(block, block.number + msg.subchain_length);
          },
          [this, &block](const primitives::Resume &msg) {
            SL_DEBUG(log_, "Resume {}", msg.subchain_length);
            return applyResume(block, block.number + msg.subchain_length);
          },
          [](auto &) {
            return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
          });
    } else if (message.consensus_engine_id == primitives::kBabeEngineId
               || message.consensus_engine_id
                      == primitives::kUnsupportedEngineId_BEEF
               ) {
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

    SL_DEBUG(log_, "Prune authority manager upto block {}", block);
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
        if (node->current_block == block) return node;
        if (directChainExists(node->current_block, block)) {
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

  ScheduleTree *AuthorityManagerImpl::findClosestAncestor(
      ForkTree<ScheduledChangeEntry> &current,
      primitives::BlockInfo const &block) const {
    auto *res = findClosestAncestor(
        const_cast<ForkTree<ScheduledChangeEntry> const &>(current), block);
    return const_cast<ForkTree<ScheduledChangeEntry> *>(res);
  }

  ScheduleTree const *AuthorityManagerImpl::findClosestAncestor(
      ForkTree<ScheduledChangeEntry> const &current,
      primitives::BlockInfo const &block) const {
    for (auto &child : current.children) {
      if (child->block.number < block.number
          && block_tree_->hasDirectChain(child->block.hash, block.hash)) {
        return findClosestAncestor(*child, block);
      }
    }
    if (current.block.number <= block.number
        && block_tree_->hasDirectChain(current.block.hash, block.hash)) {
      return &current;
    }
    return nullptr;
  }

}  // namespace kagome::authority
