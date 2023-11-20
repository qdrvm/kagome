/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/production_consensus_base.hpp"

#include <chrono>
#include <latch>

#include <boost/range/adaptor/transformed.hpp>

#include "application/app_configuration.hpp"
#include "authorship/proposer.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/block_production_error.hpp"
#include "consensus/sassafras/impl/sassafras_digests_util.hpp"
#include "consensus/sassafras/impl/sassafras_error.hpp"
#include "consensus/sassafras/sassafras_config_repository.hpp"
#include "consensus/sassafras/sassafras_lottery.hpp"
#include "consensus/sassafras/types/slot_claim.hpp"
#include "consensus/sassafras/types/slot_leadership.hpp"
#include "consensus/timeline/backoff.hpp"
#include "consensus/timeline/impl/slot_leadership_error.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "crypto/key_store/session_keys.hpp"
#include "dispute_coordinator/dispute_coordinator.hpp"
#include "metrics/histogram_timer.hpp"
#include "network/block_announce_transmitter.hpp"
#include "parachain/availability/bitfield/store.hpp"
#include "parachain/backing/store.hpp"
#include "parachain/parachain_inherent_data.hpp"
#include "primitives/digest.hpp"
#include "primitives/inherent_data.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "scale/scale.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"
#include "telemetry/service.hpp"
#include "utils/thread_pool.hpp"

using namespace std::chrono_literals;

namespace {
  inline const auto kTimestampId =
      kagome::primitives::InherentIdentifier::fromString("timstap0").value();
  inline const auto kSlotId =
      kagome::primitives::InherentIdentifier::fromString("sassslot").value();
  inline const auto kParachainId =
      kagome::primitives::InherentIdentifier::fromString("parachn0").value();

  /// The maximum allowed number of slots past the expected slot as a delay for
  /// block production. This is an intentional relaxation of block dropping algo
  static constexpr auto kMaxBlockSlotsOvertime = 2;

  constexpr const char *kIsRelayChainValidator =
      "kagome_node_is_active_validator";

  kagome::metrics::HistogramTimer metric_block_proposal_time{
      "kagome_proposer_block_constructed",
      "Time taken to construct new block",
      {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10},
  };
}  // namespace

namespace kagome::consensus {

  ProductionConsensusBase::ProductionConsensusBase(
      const application::AppConfiguration &app_config,
      const clock::SystemClock &clock,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      LazySPtr<SlotsUtil> slots_util,
      std::shared_ptr<sassafras::SassafrasConfigRepository>
          sassafras_config_repo,
      const EpochTimings &timings,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      std::shared_ptr<sassafras::SassafrasLottery> lottery,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<parachain::BitfieldStore> bitfield_store,
      std::shared_ptr<parachain::BackingStore> backing_store,
      std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator,
      std::shared_ptr<authorship::Proposer> proposer,
      primitives::events::StorageSubscriptionEnginePtr storage_sub_engine,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      std::shared_ptr<network::BlockAnnounceTransmitter> announce_transmitter,
      std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
      const ThreadPool &thread_pool,
      std::shared_ptr<boost::asio::io_context> main_thread)
      : log_(log::createLogger("Sassafras", "sassafras")),
        clock_(clock),
        block_tree_(std::move(block_tree)),
        slots_util_(std::move(slots_util)),
        sassafras_config_repo_(std::move(sassafras_config_repo)),
        timings_(timings),
        session_keys_(std::move(session_keys)),
        lottery_(std::move(lottery)),
        hasher_(std::move(hasher)),
        bitfield_store_(std::move(bitfield_store)),
        backing_store_(std::move(backing_store)),
        dispute_coordinator_(std::move(dispute_coordinator)),
        proposer_(std::move(proposer)),
        storage_sub_engine_(std::move(storage_sub_engine)),
        chain_sub_engine_(std::move(chain_sub_engine)),
        announce_transmitter_(std::move(announce_transmitter)),
        offchain_worker_api_(std::move(offchain_worker_api)),
        main_thread_(std::move(main_thread)),
        io_context_(thread_pool.io_context()),
        is_validator_by_config_(app_config.roles().flags.authority != 0),
        telemetry_{telemetry::createTelemetryService()} {
    BOOST_ASSERT(sassafras_config_repo_);
    BOOST_ASSERT(session_keys_);
    BOOST_ASSERT(lottery_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(bitfield_store_);
    BOOST_ASSERT(backing_store_);
    BOOST_ASSERT(dispute_coordinator_);
    BOOST_ASSERT(proposer_);
    BOOST_ASSERT(chain_sub_engine_);
    BOOST_ASSERT(chain_sub_engine_);
    BOOST_ASSERT(announce_transmitter_);
    BOOST_ASSERT(offchain_worker_api_);
    BOOST_ASSERT(main_thread_);
    BOOST_ASSERT(io_context_);

    // Register metrics
    metrics_registry_->registerGaugeFamily(
        kIsRelayChainValidator,
        "Tracks if the validator is in the active set. Updates at session "
        "boundary.");
    metric_is_relaychain_validator_ =
        metrics_registry_->registerGaugeMetric(kIsRelayChainValidator);
    metric_is_relaychain_validator_->set(false);
  }

  bool ProductionConsensusBase::isGenesisConsensus() const {
    primitives::BlockInfo genesis_block{0, block_tree_->getGenesisBlockHash()};
    auto res = sassafras_config_repo_->config(genesis_block, 0);
    return res.has_value();
  }

  ValidatorStatus ProductionConsensusBase::getValidatorStatus(
      const primitives::BlockInfo &block, EpochNumber epoch) const {
    auto config = sassafras_config_repo_->config(block, epoch);
    if (not config) {
      SL_CRITICAL(
          log_,
          "Can't obtain digest of epoch {} from block tree for block {}",
          epoch,
          block);
      return ValidatorStatus::NonValidator;
    }

    const auto &authorities = config.value()->authorities;
    if (session_keys_->getSassafrasKeyPair(authorities)) {
      if (authorities.size() > 1) {
        return ValidatorStatus::Validator;
      }
      return ValidatorStatus::SingleValidator;
    }

    return ValidatorStatus::NonValidator;
  }

  outcome::result<SlotNumber> ProductionConsensusBase::getSlot(
      const primitives::BlockHeader &header) const {
    return sassafras::getSlot(header);
  }

  outcome::result<void> ProductionConsensusBase::processSlot(
      SlotNumber slot, const primitives::BlockInfo &best_block) {
    auto slot_timestamp = clock_.now();

    if (slot != slots_util_.get()->timeToSlot(slot_timestamp)) {
      SL_DEBUG(log_, "Slot processing skipped: chance has missed");
      return outcome::success();
    }
    OUTCOME_TRY(epoch_number, slots_util_.get()->slotToEpoch(best_block, slot));

    // If epoch changed, generate and submit their candidate tickets along with
    // validity proofs to the blockchain
    if (lottery_->getEpoch() != epoch_number) {
      is_active_validator_ = lottery_->changeEpoch(epoch_number, best_block);
      metric_is_relaychain_validator_->set(is_active_validator_);
      if (not is_active_validator_) {
        if (is_validator_by_config_) {
          SL_VERBOSE(log_,
                     "Authority not known, skipping slot processing. "
                     "Probably authority list has changed.");
        }
      }
    }

    if (not is_active_validator_) {
      return SlotLeadershipError::NO_VALIDATOR;
    }

    Context ctx{.parent = best_block,
                .epoch = epoch_number,
                .slot = slot,
                .slot_timestamp = slot_timestamp};

    auto slot_leadership_opt =
        lottery_->getSlotLeadership(ctx.parent.hash, ctx.slot);

    if (not slot_leadership_opt.has_value()) {
      SL_TRACE(log_, "Node is not slot leader in current slot");
      return SlotLeadershipError::NO_SLOT_LEADER;
    }

    const auto &slot_leadership = slot_leadership_opt.value();
    SL_DEBUG(log_, "Sassafras author is leader in current slot");

    return processSlotLeadership(ctx, slot_leadership);
  }

  outcome::result<primitives::PreRuntime>
  ProductionConsensusBase::calculatePreDigest(
      const Context &ctx,
      const sassafras::SlotLeadership &slot_leadership) const {
    sassafras::SlotClaim slot_claim{
        .authority_index = slot_leadership.authority_index,
        .slot_number = ctx.slot,
        .signature = {},     // FIXME
        .ticket_claim = {},  // FIXME
    };

    auto encoded_slot_claim_res = scale::encode(slot_claim);
    if (encoded_slot_claim_res.has_error()) {
      SL_ERROR(
          log_, "cannot encode SlotClaim: {}", encoded_slot_claim_res.error());
      return encoded_slot_claim_res.error();
    }
    common::Buffer encoded_slot_claim{encoded_slot_claim_res.value()};

    // clang-format on
    return primitives::PreRuntime{
        {primitives::kSassafrasEngineId, encoded_slot_claim}};
  }

  outcome::result<primitives::Seal> ProductionConsensusBase::sealBlock(
      const Context &ctx, const primitives::Block &block) const {
    BOOST_ASSERT(ctx.keypair != nullptr);

    // Calculate and save hash, 'cause it's new produced block
    // Note: it is temporary hash significant for signing
    primitives::calculateBlockHash(
        const_cast<primitives::BlockHeader &>(block.header), *hasher_);

    // auto signature_res =
    //     sr25519_provider_->sign(*ctx.keypair, block.header.hash());
    // if (signature_res.has_value()) {
    //   Seal seal{.signature = signature_res.value()};
    //   auto encoded_seal = common::Buffer(scale::encode(seal).value());
    //   return primitives::Seal{{primitives::kBabeEngineId, encoded_seal}};
    // }
    //
    // SL_ERROR(log_, "Error signing a block seal: {}", signature_res.error());
    // return signature_res.as_failure();
    return primitives::Seal{};
  }

  outcome::result<void> ProductionConsensusBase::processSlotLeadership(
      const Context &ctx, const sassafras::SlotLeadership &slot_leadership) {
    auto parent_header_res = block_tree_->getBlockHeader(ctx.parent.hash);
    BOOST_ASSERT_MSG(parent_header_res.has_value(),
                     "The best block is always known");
    auto &parent_header = parent_header_res.value();

    if (backoff(*this,
                parent_header,
                block_tree_->getLastFinalized().number,
                ctx.slot)) {
      SL_INFO(log_,
              "Backing off claiming new slot for block authorship: "
              "finality is lagging.");
      return SlotLeadershipError::BACKING_OFF;
    }

    BOOST_ASSERT(slot_leadership.keypair != nullptr);

    // build a block to be announced
    SL_VERBOSE(log_,
               "Obtained slot leadership in slot {} epoch {}",
               ctx.slot,
               ctx.epoch);

    SL_INFO(log_, "Sassafras builds block on top of block {}", ctx.parent);

    primitives::InherentData inherent_data;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   ctx.slot_timestamp.time_since_epoch())
                   .count();

    if (auto res = inherent_data.putData<uint64_t>(kTimestampId, now);
        res.has_error()) {
      SL_ERROR(log_, "cannot put an inherent data: {}", res.error());
      return BlockProductionError::CAN_NOT_PREPARE_BLOCK;
    }

    if (auto res = inherent_data.putData(kSlotId, ctx.slot); res.has_error()) {
      SL_ERROR(log_, "cannot put an inherent data: {}", res.error());
      return BlockProductionError::CAN_NOT_PREPARE_BLOCK;
    }

    parachain::ParachainInherentData parachain_inherent_data;

    {
      auto &relay_parent = ctx.parent.hash;
      parachain_inherent_data.bitfields =
          bitfield_store_->getBitfields(relay_parent);

      parachain_inherent_data.backed_candidates =
          backing_store_->get(relay_parent);
      SL_TRACE(log_,
               "Get backed candidates from store.(count={}, relay_parent={})",
               parachain_inherent_data.backed_candidates.size(),
               relay_parent);

      parachain_inherent_data.parent_header = std::move(parent_header);

      {  // Fill disputes
        std::latch latch(1);
        dispute_coordinator_->getDisputeForInherentData(
            ctx.parent, [&](auto res) {
              parachain_inherent_data.disputes = std::move(res);
              latch.count_down();
            });
        latch.wait();
      }
    }

    if (auto res = inherent_data.putData(kParachainId, parachain_inherent_data);
        res.has_error()) {
      SL_ERROR(log_, "cannot put an inherent data: {}", res.error());
      return BlockProductionError::CAN_NOT_PREPARE_BLOCK;
    }

    auto proposal_start = std::chrono::steady_clock::now();

    auto pre_digest_res = calculatePreDigest(ctx, slot_leadership);
    if (pre_digest_res.has_error()) {
      SL_ERROR(log_, "cannot propose a block: {}", pre_digest_res.error());
      return BlockProductionError::CAN_NOT_PREPARE_BLOCK;
    }
    const auto &pre_digest = pre_digest_res.value();

    auto propose = [this,
                    self{shared_from_this()},
                    inherent_data{std::move(inherent_data)},
                    now,
                    proposal_start,
                    pre_digest{std::move(pre_digest)},
                    ctx]() mutable {
      auto changes_tracker =
          std::make_shared<storage::changes_trie::StorageChangesTrackerImpl>();

      // create new block
      auto res = proposer_->propose(ctx.parent,
                                    slots_util_.get()->slotFinishTime(ctx.slot)
                                        - timings_.slot_duration / 3,
                                    inherent_data,
                                    {pre_digest},
                                    changes_tracker);
      if (not res) {
        SL_ERROR(log_, "Cannot propose a block: {}", res.error());
        return;
      }
      auto &unsealed_block = res.value();
      auto proposed = [self,
                       now,
                       proposal_start,
                       changes_tracker{std::move(changes_tracker)},
                       unsealed_block{std::move(unsealed_block)},
                       ctx]() mutable {
        auto res =
            self->processSlotLeadershipProposed(ctx,
                                                now,
                                                proposal_start,
                                                std::move(changes_tracker),
                                                std::move(unsealed_block));
        if (res.has_error()) {
          SL_ERROR(self->log_, "Cannot propose a block: {}", res.error());
          return;
        }
      };
      main_thread_->post(std::move(proposed));
    };

    io_context_->post(std::move(propose));

    return outcome::success();
  }

  outcome::result<void> ProductionConsensusBase::processSlotLeadershipProposed(
      const Context &ctx,
      uint64_t now,
      clock::SteadyClock::TimePoint proposal_start,
      std::shared_ptr<storage::changes_trie::StorageChangesTrackerImpl>
          &&changes_tracker,
      primitives::Block &&block) {
    auto duration_ms =
        metric_block_proposal_time.observe(proposal_start).count();
    SL_DEBUG(log_, "Block has been built in {} ms", duration_ms);

    // Ensure block's extrinsics root matches extrinsics in block's body
    BOOST_ASSERT_MSG(
        [&block]() {
          using boost::adaptors::transformed;
          const auto &ext_root_res = storage::trie::calculateOrderedTrieHash(
              storage::trie::StateVersion::V0,
              block.body | transformed([](const auto &ext) {
                return common::Buffer{scale::encode(ext).value()};
              }));
          return ext_root_res.has_value()
             and (ext_root_res.value() == block.header.extrinsics_root);
        }(),
        "Extrinsics root does not match extrinsics in the block");

    // seal the block
    auto seal_res = sealBlock(ctx, block);
    if (!seal_res) {
      SL_ERROR(log_, "Failed to seal the block: {}", seal_res.error());
      return BlockProductionError::CAN_NOT_SEAL_BLOCK;
    }

    // add seal digest item
    block.header.digest.emplace_back(seal_res.value());

    // Calculate and save hash, 'cause seal digest was added
    primitives::calculateBlockHash(block.header, *hasher_);

    if (clock_.now() > slots_util_.get()->slotFinishTime(
            ctx.slot + kMaxBlockSlotsOvertime)) {
      SL_WARN(log_,
              "Block was not built on time. "
              "Allowed slots ({}) have passed. "
              "If you are executing in debug mode, consider to rebuild in "
              "release",
              kMaxBlockSlotsOvertime);
      return BlockProductionError::WAS_NOT_BUILD_ON_TIME;
    }

    const auto block_info = block.header.blockInfo();

    auto previous_best_block = block_tree_->bestBlock();

    // add block to the block tree
    if (auto add_res = block_tree_->addBlock(block); not add_res) {
      SL_ERROR(log_, "Could not add block {}: {}", block_info, add_res.error());
      return BlockProductionError::CAN_NOT_SAVE_BLOCK;
    }

    changes_tracker->onBlockAdded(
        block_info.hash, storage_sub_engine_, chain_sub_engine_);

    telemetry_->notifyBlockImported(block_info, telemetry::BlockOrigin::kOwn);
    telemetry_->pushBlockStats();

    // finally, broadcast the sealed block
    announce_transmitter_->blockAnnounce(network::BlockAnnounce{
        block.header,
        block_info == block_tree_->bestBlock() ? network::BlockState::Best
                                               : network::BlockState::Normal,
        common::Buffer{},
    });
    SL_DEBUG(
        log_,
        "Announced block number {} in slot {} (epoch {}) with timestamp {}",
        block.header.number,
        ctx.slot,
        ctx.epoch,
        now);

    auto current_best_block = block_tree_->bestBlock();

    // Create a new offchain worker for block if it is best only
    if (current_best_block.number > previous_best_block.number) {
      auto ocw_res = offchain_worker_api_->offchain_worker(
          block.header.parent_hash, block.header);
      if (ocw_res.has_failure()) {
        log_->error("Can't spawn offchain worker for block {}: {}",
                    block_info,
                    ocw_res.error());
      }
    }

    return outcome::success();
  }

}  // namespace kagome::consensus
