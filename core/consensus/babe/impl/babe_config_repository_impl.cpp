/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babe_config_repository_impl.hpp"

#include "application/app_state_manager.hpp"
#include "babe_digests_util.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "consensus/babe/babe_error.hpp"
#include "crypto/hasher.hpp"
#include "primitives/block_header.hpp"
#include "runtime/runtime_api/babe_api.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::consensus::babe {

  BabeConfigRepositoryImpl::BabeConfigRepositoryImpl(
      const std::shared_ptr<application::AppStateManager> &app_state_manager,
      std::shared_ptr<storage::SpacedStorage> persistent_storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<runtime::BabeApi> babe_api,
      std::shared_ptr<crypto::Hasher> hasher,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      const BabeClock &clock)
      : persistent_storage_(persistent_storage->getSpace(storage::Space::kDefault)),
        block_tree_(std::move(block_tree)),
        header_repo_(std::move(header_repo)),
        babe_api_(std::move(babe_api)),
        hasher_(std::move(hasher)),
        chain_sub_([&] {
          BOOST_ASSERT(chain_events_engine != nullptr);
          return std::make_shared<primitives::events::ChainEventSubscriber>(
              chain_events_engine);
        }()),
        clock_(clock),
        logger_(log::createLogger("BabeConfigRepo", "babe_config_repo")) {
    BOOST_ASSERT(persistent_storage_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(babe_api_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    BOOST_ASSERT(app_state_manager != nullptr);
    app_state_manager->atPrepare([this] { return prepare(); });
  }

  bool BabeConfigRepositoryImpl::prepare() {
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

  outcome::result<void> BabeConfigRepositoryImpl::load() {
    const auto finalized_block = block_tree_->getLastFinalized();

    // First, look up slot number of block number 1 sync epochs
    if (finalized_block.number > 0) {
      OUTCOME_TRY(first_block_header, block_tree_->getBlockHeader(1));

      auto babe_digest_res = getBabeDigests(first_block_header);
      BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                       "Any non genesis block must contain babe digest");
      auto first_slot_number = babe_digest_res.value().second.slot_number;

      syncEpoch([&] { return std::tuple(first_slot_number, true); });
    }

    // 1. Load last state
    OUTCOME_TRY(encoded_last_state_opt,
                persistent_storage_->tryGet(
                    storage::kBabeConfigRepoStateLookupKey("last")));

    if (encoded_last_state_opt.has_value()) {
      auto last_state_res = scale::decode<std::shared_ptr<BabeConfigNode>>(
          encoded_last_state_opt.value());

      if (last_state_res.has_value()) {
        auto &last_state = last_state_res.value();
        if (last_state->block.number <= finalized_block.number) {
          root_ = std::move(last_state);
          SL_DEBUG(logger_,
                   "State was initialized by last saved on block {}",
                   root_->block);
        } else {
          SL_WARN(
              logger_,
              "Last state not match with last finalized; Try to use savepoint");
        }
      } else {
        SL_WARN(
            logger_, "Can not decode last state: {}", last_state_res.error());
        std::ignore = persistent_storage_->remove(
            storage::kBabeConfigRepoStateLookupKey("last"));
      }
    }

    // 2. Load from last control point, if state is still not found
    if (root_ == nullptr) {
      for (auto block_number =
               (finalized_block.number / kSavepointBlockInterval)
               * kSavepointBlockInterval;
           block_number > 0;
           block_number -= kSavepointBlockInterval) {
        OUTCOME_TRY(encoded_saved_state_opt,
                    persistent_storage_->tryGet(
                        storage::kBabeConfigRepoStateLookupKey(block_number)));

        if (not encoded_saved_state_opt.has_value()) {
          continue;
        }

        auto saved_state_res = scale::decode<std::shared_ptr<BabeConfigNode>>(
            encoded_saved_state_opt.value());

        if (saved_state_res.has_error()) {
          SL_WARN(logger_,
                  "Can not decode state saved on block {}: {}",
                  block_number,
                  saved_state_res.error());
          std::ignore = persistent_storage_->remove(
              storage::kBabeConfigRepoStateLookupKey(block_number));
          continue;
        }

        root_ = std::move(saved_state_res.value());
        SL_VERBOSE(logger_,
                   "State was initialized by savepoint on block {}",
                   root_->block);
        break;
      }
    }

    // 3. Load state from genesis, if state is still not found
    if (root_ == nullptr) {
      auto genesis_hash = block_tree_->getGenesisBlockHash();
      auto babe_config_res = babe_api_->configuration(genesis_hash);
      if (babe_config_res.has_error()) {
        SL_WARN(logger_,
                "Can't get babe config over babe API on genesis block: {}",
                babe_config_res.error());
        return babe_config_res.as_failure();
      }
      auto &babe_config = babe_config_res.value();

      root_ = BabeConfigNode::createAsRoot(
          {0, genesis_hash},
          std::make_shared<primitives::BabeConfiguration>(
              std::move(babe_config)));
      SL_VERBOSE(logger_, "State was initialized by genesis block");
    }

    BOOST_ASSERT_MSG(root_ != nullptr, "The root must be initialized by now");

    // Init slot duration and epoch length
    auto slot_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        root_->config->slot_duration);
    BOOST_ASSERT_MSG(slot_duration.count() > 0,
                     "Slot duration must be greater zero");
    const_cast<BabeDuration &>(slot_duration_) = slot_duration;
    auto epoch_length = root_->config->epoch_length;
    BOOST_ASSERT_MSG(epoch_length, "Epoch length must be greater zero");
    const_cast<EpochLength &>(epoch_length_) = epoch_length;

    // 4. Apply digests before last finalized
    bool need_to_save = false;
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

      primitives::BlockContext context{.block = {block_number, block_hash}};

      for (auto &item : block_header.digest) {
        auto res = visit_in_place(
            item,
            [&](const primitives::PreRuntime &msg) -> outcome::result<void> {
              if (msg.consensus_engine_id == primitives::kBabeEngineId) {
                OUTCOME_TRY(digest_item,
                            scale::decode<BabeBlockHeader>(msg.data));

                return onDigest(context, digest_item);
              }
              return outcome::success();
            },
            [&](const primitives::Consensus &msg) -> outcome::result<void> {
              if (msg.consensus_engine_id == primitives::kBabeEngineId) {
                OUTCOME_TRY(digest_item,
                            scale::decode<primitives::BabeDigest>(msg.data));

                return onDigest(context, digest_item);
              }
              return outcome::success();
            },
            [](const auto &) { return outcome::success(); });
        if (res.has_error()) {
          SL_WARN(logger_,
                  "Can't apply babe digest of finalized block {}: {}",
                  context.block,
                  res.error());
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
                                        primitives::BabeDigest>>>
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
                  auto res = scale::decode<primitives::BabeDigest>(msg.data);
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

  outcome::result<void> BabeConfigRepositoryImpl::save() {
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
                          : ancestor_node->makeDescendant(savepoint_block);
          auto res = persistent_storage_->put(
              storage::kBabeConfigRepoStateLookupKey(new_savepoint),
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
        storage::kBabeConfigRepoStateLookupKey("last"),
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

  std::shared_ptr<const primitives::BabeConfiguration>
  BabeConfigRepositoryImpl::config(const primitives::BlockContext &context,
                                   EpochNumber epoch_number) {
    auto node = getNode(context);
    if (node) {
      if (epoch_number > node->epoch) {
        return node->next_config.value_or(node->config);
      }
      return node->config;
    }
    return {};
  }

  BabeDuration BabeConfigRepositoryImpl::slotDuration() const {
    BOOST_ASSERT_MSG(slot_duration_ != BabeDuration::zero(),
                     "Slot duration is not initialized");
    return slot_duration_;
  }

  EpochLength BabeConfigRepositoryImpl::epochLength() const {
    BOOST_ASSERT_MSG(epoch_length_ != 0, "Epoch length is not initialized");
    return epoch_length_;
  }

  outcome::result<void> BabeConfigRepositoryImpl::onDigest(
      const primitives::BlockContext &context,
      const consensus::babe::BabeBlockHeader &digest) {
    EpochNumber epoch_number = slotToEpoch(digest.slot_number);

    auto node = getNode(context);
    BOOST_ASSERT(node != nullptr);

    SL_LOG(logger_,
           node->epoch != epoch_number ? log::Level::DEBUG : log::Level::TRACE,
           "BabeBlockHeader babe-digest on block {}: "
           "slot {}, epoch {}, authority #{}, {}",
           context.block,
           digest.slot_number,
           epoch_number,
           digest.authority_index,
           to_string(digest.slotType()));

    if (node->block == context.block) {
      return BabeError::BAD_ORDER_OF_DIGEST_ITEM;
    }

    // Create descendant if and only if epoch is changed
    if (node->epoch != epoch_number) {
      auto new_node = node->makeDescendant(context.block, epoch_number);

      node->descendants.emplace_back(std::move(new_node));
    }

    return outcome::success();
  }

  outcome::result<void> BabeConfigRepositoryImpl::onDigest(
      const primitives::BlockContext &context,
      const primitives::BabeDigest &digest) {
    return visit_in_place(
        digest,
        [&](const primitives::NextEpochData &msg) -> outcome::result<void> {
          SL_DEBUG(logger_,
                   "NextEpochData babe-digest on block {}: "
                   "{} authorities, randomness {}",
                   context.block,
                   msg.authorities.size(),
                   msg.randomness);
          return onNextEpochData(context, msg);
        },
        [&](const primitives::OnDisabled &msg) {
          SL_TRACE(
              logger_,
              "OnDisabled babe-digest on block {}: "
              "disable authority #{}; ignored (it is checked only by runtime)",
              context.block,
              msg.authority_index);
          // Implemented sending of OnDisabled events before actually preventing
          // disabled validators from authoring, so it's possible that there are
          // blocks on the chain that came from disabled validators (before they
          // were booted from the set at the end of epoch). Currently, the
          // runtime prevents disabled validators from authoring (it will just
          // panic), so we don't do any client-side handling in substrate
          // https://matrix.to/#/!oZltgdfyakVMtEAWCI:web3.foundation/$hArAlUKaxvquGdaRG9W8ihcsNrO6wD4Q2CQjDIb3MMY?via=web3.foundation&via=matrix.org&via=matrix.parity.io
          return outcome::success();
        },
        [&](const primitives::NextConfigData &msg) {
          return visit_in_place(
              msg,
              [&](const primitives::NextConfigDataV1 &msg) {
                SL_DEBUG(logger_,
                         "NextConfigData babe-digest on block {}: "
                         "ratio={}/{}, second_slot={}",
                         context.block,
                         msg.ratio.first,
                         msg.ratio.second,
                         to_string(msg.second_slot));
                return onNextConfigData(context, msg);
              },
              [&](const auto &) {
                SL_WARN(logger_,
                        "Unsupported NextConfigData babe-digest on block {}: "
                        "variant #{}",
                        context.block,
                        digest.which());
                return BabeError::UNKNOWN_DIGEST_TYPE;
              });
        },
        [&](auto &) {
          SL_WARN(logger_,
                  "Unsupported babe-digest on block {}: variant #{}",
                  context.block,
                  digest.which());
          return BabeError::UNKNOWN_DIGEST_TYPE;
        });
  }

  outcome::result<void> BabeConfigRepositoryImpl::onNextEpochData(
      const primitives::BlockContext &context,
      const primitives::NextEpochData &msg) {
    auto node = getNode(context);

    if (node->block != context.block) {
      return BabeError::BAD_ORDER_OF_DIGEST_ITEM;
    }

    auto config = node->next_config.value_or(node->config);

    if (config->authorities != msg.authorities
        or config->randomness != msg.randomness) {
      auto new_config =
          std::make_shared<primitives::BabeConfiguration>(*config);
      new_config->authorities = msg.authorities;
      new_config->randomness = msg.randomness;
      node->next_config = std::move(new_config);
    }

    return outcome::success();
  }

  outcome::result<void> BabeConfigRepositoryImpl::onNextConfigData(
      const primitives::BlockContext &context,
      const primitives::NextConfigDataV1 &msg) {
    auto node = getNode(context);

    if (node->block != context.block) {
      return BabeError::BAD_ORDER_OF_DIGEST_ITEM;
    }

    auto config = node->next_config.value_or(node->config);

    if (config->leadership_rate != msg.ratio
        or config->allowed_slots != msg.second_slot) {
      auto new_config =
          std::make_shared<primitives::BabeConfiguration>(*config);
      new_config->leadership_rate = msg.ratio;
      new_config->allowed_slots = msg.second_slot;
      node->next_config = std::move(new_config);
    }

    return outcome::success();
  }

  std::shared_ptr<BabeConfigNode> BabeConfigRepositoryImpl::getNode(
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

    std::shared_ptr<BabeConfigNode> ancestor = root_;
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

  bool BabeConfigRepositoryImpl::directChainExists(
      const primitives::BlockInfo &ancestor,
      const primitives::BlockInfo &descendant) const {
    SL_TRACE(logger_,
             "Looking if direct chain exists between {} and {}",
             ancestor,
             descendant);
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
    return result;
  }

  void BabeConfigRepositoryImpl::prune(const primitives::BlockInfo &block) {
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

    if (node->block != block) {
      // Reorganize ancestry
      auto new_node = node->makeDescendant(block);
      auto descendants = std::move(node->descendants);
      for (auto &descendant : descendants) {
        if (directChainExists(block, descendant->block)) {
          new_node->descendants.emplace_back(std::move(descendant));
        }
      }
      node = std::move(new_node);
    }

    root_ = std::move(node);

    SL_TRACE(logger_, "Prune upto block {}", block);
  }

  void BabeConfigRepositoryImpl::cancel(const primitives::BlockInfo &block) {
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
      ancestor =
          std::const_pointer_cast<BabeConfigNode>(ancestor->parent.lock());
      BOOST_ASSERT_MSG(ancestor != nullptr, "Non root node must have a parent");
    }

    auto it = std::find_if(ancestor->descendants.begin(),
                           ancestor->descendants.end(),
                           [&](std::shared_ptr<BabeConfigNode> node) {
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

  BabeSlotNumber BabeConfigRepositoryImpl::syncEpoch(
      std::function<std::tuple<BabeSlotNumber, bool>()> &&f) {
    if (not is_first_block_finalized_) {
      auto [first_block_slot_number, is_first_block_finalized] = f();
      first_block_slot_number_.emplace(first_block_slot_number);
      is_first_block_finalized_ = is_first_block_finalized;
      SL_TRACE(
          logger_,
          "Epoch beginning is synchronized: first block slot number is {} now",
          first_block_slot_number_.value());
    }
    return first_block_slot_number_.value();
  }

  BabeSlotNumber BabeConfigRepositoryImpl::getCurrentSlot() const {
    return static_cast<BabeSlotNumber>(clock_.now().time_since_epoch()
                                       / slotDuration());
  }

  BabeTimePoint BabeConfigRepositoryImpl::slotStartTime(
      BabeSlotNumber slot) const {
    return clock_.zero() + slot * slotDuration();
  }

  BabeDuration BabeConfigRepositoryImpl::remainToStartOfSlot(
      BabeSlotNumber slot) const {
    auto deadline = slotStartTime(slot);
    auto now = clock_.now();
    if (deadline > now) {
      return deadline - now;
    }
    return BabeDuration{};
  }

  BabeTimePoint BabeConfigRepositoryImpl::slotFinishTime(
      BabeSlotNumber slot) const {
    return slotStartTime(slot + 1);
  }

  BabeDuration BabeConfigRepositoryImpl::remainToFinishOfSlot(
      BabeSlotNumber slot) const {
    return remainToStartOfSlot(slot + 1);
  }

  BabeSlotNumber BabeConfigRepositoryImpl::getFirstBlockSlotNumber() {
    if (first_block_slot_number_.has_value()) {
      return first_block_slot_number_.value();
    }

    return getCurrentSlot();
  }

  EpochNumber BabeConfigRepositoryImpl::slotToEpoch(BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeConfigRepositoryImpl &>(*this).getFirstBlockSlotNumber();
    if (slot > genesis_slot_number) {
      return (slot - genesis_slot_number) / epochLength();
    }
    return 0;
  }

  BabeSlotNumber BabeConfigRepositoryImpl::slotInEpoch(
      BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeConfigRepositoryImpl &>(*this).getFirstBlockSlotNumber();
    if (slot > genesis_slot_number) {
      return (slot - genesis_slot_number) % epochLength();
    }
    return 0;
  }

}  // namespace kagome::consensus::babe
