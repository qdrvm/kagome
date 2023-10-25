/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/sassafras/impl/sassafras.hpp"

#include <chrono>
#include <latch>

#include "application/app_configuration.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/sassafras/impl/sassafras_digests_util.hpp"
#include "consensus/sassafras/impl/sassafras_error.hpp"
#include "consensus/sassafras/impl/threshold_util.hpp"
#include "consensus/sassafras/sassafras_config_repository.hpp"
#include "consensus/sassafras/sassafras_lottery.hpp"
#include "consensus/sassafras/types/seal.hpp"
#include "consensus/sassafras/types/slot_claim.hpp"
#include "consensus/timeline/backoff.hpp"
#include "consensus/timeline/impl/block_production_error.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "dispute_coordinator/dispute_coordinator.hpp"
#include "metrics/histogram_timer.hpp"
#include "parachain/availability/bitfield/store.hpp"
#include "parachain/backing/store.hpp"
#include "parachain/parachain_inherent_data.hpp"
#include "primitives/digest.hpp"
#include "primitives/inherent_data.hpp"
#include "scale.hpp"

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
  // static constexpr auto kMaxBlockSlotsOvertime = 2
  constexpr const char *kIsRelayChainValidator =
      "kagome_node_is_active_validator";

  kagome::metrics::HistogramTimer metric_block_proposal_time{
      "kagome_proposer_block_constructed",
      "Time taken to construct new block",
      {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10},
  };
}  // namespace

namespace kagome::consensus::sassafras {

  Sassafras::Sassafras(
      const application::AppConfiguration &app_config,
      const clock::SystemClock &clock,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      LazySPtr<SlotsUtil> slots_util,
      std::shared_ptr<SassafrasConfigRepository> sassafras_config_repo,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      std::shared_ptr<SassafrasLottery> lottery,
      std::shared_ptr<parachain::BitfieldStore> bitfield_store,
      std::shared_ptr<parachain::BackingStore> backing_store,
      std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator)
      : log_(log::createLogger("Sassafras", "sassafras")),
        clock_(clock),
        block_tree_(std::move(block_tree)),
        slots_util_(std::move(slots_util)),
        sassafras_config_repo_(std::move(sassafras_config_repo)),
        session_keys_(std::move(session_keys)),
        lottery_(std::move(lottery)),
        bitfield_store_(std::move(bitfield_store)),
        backing_store_(std::move(backing_store)),
        dispute_coordinator_(std::move(dispute_coordinator)),
        is_validator_by_config_(app_config.roles().flags.authority != 0),
        telemetry_{telemetry::createTelemetryService()} {
    BOOST_ASSERT(sassafras_config_repo_);
    BOOST_ASSERT(session_keys_);
    BOOST_ASSERT(lottery_);
    BOOST_ASSERT(bitfield_store_);
    BOOST_ASSERT(backing_store_);
    BOOST_ASSERT(dispute_coordinator_);

    // Register metrics
    metrics_registry_->registerGaugeFamily(
        kIsRelayChainValidator,
        "Tracks if the validator is in the active set. Updates at session "
        "boundary.");
    metric_is_relaychain_validator_ =
        metrics_registry_->registerGaugeMetric(kIsRelayChainValidator);
    metric_is_relaychain_validator_->set(false);
  }

  ValidatorStatus Sassafras::getValidatorStatus(
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

  std::tuple<Duration, EpochLength> Sassafras::getTimings() const {
    return {sassafras_config_repo_->slotDuration(),
            sassafras_config_repo_->epochLength()};
  }

  outcome::result<SlotNumber> Sassafras::getSlot(
      const primitives::BlockHeader &header) const {
    return sassafras::getSlot(header);
  }

  outcome::result<void> Sassafras::processSlot(
      SlotNumber slot, const primitives::BlockInfo &best_block) {
    auto slot_timestamp = clock_.now();

    if (slot != slots_util_.get()->timeToSlot(slot_timestamp)) {
      SL_DEBUG(log_, "Slot processing skipped: chance has missed");
      return outcome::success();
    }
    OUTCOME_TRY(epoch_number, slots_util_.get()->slotToEpoch(best_block, slot));

    auto config_res = sassafras_config_repo_->config(best_block, epoch_number);
    [[unlikely]] if (not config_res.has_value()) {
      SL_ERROR(log_,
               "Can not get epoch: {}; Skipping slot processing",
               config_res.error());
      return config_res.as_failure();
    }
    auto &config = *config_res.value();

    auto keypair = session_keys_->getBabeKeyPair(config.authorities);
    if (not keypair) {
      metric_is_relaychain_validator_->set(false);
      if (is_validator_by_config_) {
        SL_VERBOSE(log_,
                   "Authority not known, skipping slot processing. "
                   "Probably authority list has changed.");
      }
      return BlockProductionError::NO_VALIDATOR;
    }

    Context ctx{.parent = best_block,
                .epoch = epoch_number,
                .slot = slot,
                .slot_timestamp = slot_timestamp,
                .keypair = std::move(keypair->first)};

    metric_is_relaychain_validator_->set(true);
    const auto &authority_index = keypair->second;

    // If epoch changed, generate and submit their candidate tickets along with
    // validity proofs to the blockchain
    if (lottery_->getEpoch() != epoch_number) {
      // TODO Generate and submit tickets here
      changeLotteryEpoch(ctx, epoch_number, authority_index, config);
    }

    auto slot_leadership =
        lottery_->getSlotLeadership(ctx.parent.hash, ctx.slot);

    if (slot_leadership.has_value()) {
      const auto &vrf_result = slot_leadership.value();
      SL_DEBUG(log_,
               "Babe author {} is primary slot-leader "
               "(vrfOutput: {}, proof: {})",
               ctx.keypair->public_key,
               common::Buffer(vrf_result.output),
               common::Buffer(vrf_result.proof));

      return processSlotLeadership(
          ctx, slot_timestamp, std::cref(vrf_result), authority_index);
    }

    SL_TRACE(log_,
             "Validator {} is not slot leader in current slot",
             ctx.keypair->public_key);

    return BlockProductionError::NO_SLOT_LEADER;
  }

  outcome::result<primitives::PreRuntime> Sassafras::calculatePreDigest(
      const Context &ctx,
      std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
      primitives::AuthorityIndex authority_index) const {
    SlotClaim slot_claim{
        .authority_index = authority_index,
        .slot_number = ctx.slot,
        .signature = {},     // FIXME
        .ticket_claim = {},  // FIXME
    };

    // if (slot_claim.needVRFCheck()) {
    //   if (not output.has_value()) {
    //     SL_ERROR(
    //       log_,
    //       "VRF proof is required to build block header but was not passed");
    //     return BabeError::MISSING_PROOF;
    //   }
    //   slot_claim.vrf_output = output.value();
    // }

    auto encoded_slot_claim_res = scale::encode(slot_claim);
    if (encoded_slot_claim_res.has_error()) {
      SL_ERROR(
          log_, "cannot encode SlotClaim: {}", encoded_slot_claim_res.error());
      return encoded_slot_claim_res.error();
    }
    common::Buffer encoded_slot_claim{encoded_slot_claim_res.value()};

    // clang-format on
    return primitives::PreRuntime{
        {primitives::kBabeEngineId, encoded_slot_claim}};
  }

  outcome::result<primitives::Seal> Sassafras::sealBlock(
      const Context &ctx, const primitives::Block &block) const {
    // BOOST_ASSERT(ctx.keypair != nullptr);
    //
    // // Calculate and save hash, 'cause it's new produced block
    // // Note: it is temporary hash significant for signing
    // primitives::calculateBlockHash(
    //     const_cast<primitives::BlockHeader &>(block.header), *hasher_);
    //
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

  outcome::result<void> Sassafras::processSlotLeadership(
      const Context &ctx,
      TimePoint slot_timestamp,
      std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
      primitives::AuthorityIndex authority_index) {
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
      return BlockProductionError::BACKING_OFF;
    }

    BOOST_ASSERT(ctx.keypair != nullptr);

    // build a block to be announced
    SL_VERBOSE(log_,
               "Obtained slot leadership in slot {} epoch {}",
               ctx.slot,
               ctx.epoch);

    SL_INFO(log_, "Babe builds block on top of block {}", ctx.parent);

    primitives::InherentData inherent_data;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   slot_timestamp.time_since_epoch())
                   .count();

    if (auto res = inherent_data.putData<uint64_t>(kTimestampId, now);
        res.has_error()) {
      SL_ERROR(log_, "cannot put an inherent data: {}", res.error());
      return SassafrasError::CAN_NOT_PREPARE_BLOCK;
    }

    if (auto res = inherent_data.putData(kSlotId, ctx.slot); res.has_error()) {
      SL_ERROR(log_, "cannot put an inherent data: {}", res.error());
      return SassafrasError::CAN_NOT_PREPARE_BLOCK;
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
      return SassafrasError::CAN_NOT_PREPARE_BLOCK;
    }

    // clang-format off

// auto proposal_start = std::chrono::steady_clock::now();
// // calculate babe_pre_digest
// auto babe_pre_digest_res =
//     babePreDigest(ctx, slot_type, output, authority_index);
// if (not babe_pre_digest_res) {
//   SL_ERROR(log_, "cannot propose a block: {}",
//   babe_pre_digest_res.error()); return
//   BabeError::CAN_NOT_PREPARE_BLOCK;
// }
// const auto &babe_pre_digest = babe_pre_digest_res.value();
//
// auto propose = [this,
//                 self{shared_from_this()},
//                 inherent_data{std::move(inherent_data)},
//                 now,
//                 proposal_start,
//                 babe_pre_digest{std::move(babe_pre_digest)},
//                 ctx]() mutable {
//   auto changes_tracker =
//       std::make_shared<storage::changes_trie::StorageChangesTrackerImpl>();
//
//   // create new block
//   auto res = proposer_->propose(ctx.parent,
//                                 slots_util_.get()->slotFinishTime(ctx.slot)
//                                     -
//                                     babe_config_repo_->slotDuration()
//                                     / 3,
//                                 inherent_data,
//                                 {babe_pre_digest},
//                                 changes_tracker);
//   if (not res) {
//     SL_ERROR(log_, "Cannot propose a block: {}", res.error());
//     return;
//   }
//   auto &unsealed_block = res.value();
//   auto proposed = [self,
//                    now,
//                    proposal_start,
//                    changes_tracker{std::move(changes_tracker)},
//                    unsealed_block{std::move(unsealed_block)},
//                    ctx]() mutable {
//     auto res =
//         self->processSlotLeadershipProposed(ctx,
//                                             now,
//                                             proposal_start,
//                                             std::move(changes_tracker),
//                                             std::move(unsealed_block));
//     if (res.has_error()) {
//       SL_ERROR(self->log_, "Cannot propose a block: {}", res.error());
//       return;
//     }
//   };
//   main_thread_->post(std::move(proposed));
// };
//
// io_context_->post(std::move(propose));

    // clang-format on
    return outcome::success();
  }

  //  outcome::result<void> Sassafras::processSlotLeadershipProposed(
  //      const Context &ctx,
  //      uint64_t now,
  //      clock::SteadyClock::TimePoint proposal_start,
  //      std::shared_ptr<storage::changes_trie::StorageChangesTrackerImpl>
  //          &&changes_tracker,
  //      primitives::Block &&block) {
  //    auto duration_ms =
  //        metric_block_proposal_time.observe(proposal_start).count();
  //    SL_DEBUG(log_, "Block has been built in {} ms", duration_ms);
  //
  //    // Ensure block's extrinsics root matches extrinsics in block's body
  //    BOOST_ASSERT_MSG(
  //        [&block]() {
  //          using boost::adaptors::transformed;
  //          const auto &ext_root_res =
  //          storage::trie::calculateOrderedTrieHash(
  //              storage::trie::StateVersion::V0,
  //              block.body | transformed([](const auto &ext) {
  //                return common::Buffer{scale::encode(ext).value()};
  //              }));
  //          return ext_root_res.has_value()
  //             and (ext_root_res.value() == block.header.extrinsics_root);
  //        }(),
  //        "Extrinsics root does not match extrinsics in the block");
  //
  //    // seal the block
  //    auto seal_res = sealBlock(ctx, block);
  //    if (!seal_res) {
  //      SL_ERROR(log_, "Failed to seal the block: {}", seal_res.error());
  //      return BabeError::CAN_NOT_SEAL_BLOCK;
  //    }
  //
  //    // add seal digest item
  //    block.header.digest.emplace_back(seal_res.value());
  //
  //    // Calculate and save hash, 'cause seal digest was added
  //    primitives::calculateBlockHash(block.header, *hasher_);
  //
  //    if (clock_.now() > slots_util_.get()->slotFinishTime(
  //            ctx.slot + kMaxBlockSlotsOvertime)) {
  //      SL_WARN(log_,
  //              "Block was not built on time. "
  //              "Allowed slots ({}) have passed. "
  //              "If you are executing in debug mode, consider to rebuild in "
  //              "release",
  //              kMaxBlockSlotsOvertime);
  //      return BabeError::WAS_NOT_BUILD_ON_TIME;
  //    }
  //
  //    const auto block_info = block.header.blockInfo();
  //
  //    auto previous_best_block = block_tree_->bestBlock();
  //
  //    // add block to the block tree
  //    if (auto add_res = block_tree_->addBlock(block); not add_res) {
  //      SL_ERROR(log_, "Could not add block {}: {}", block_info,
  //      add_res.error()); auto removal_res =
  //      block_tree_->removeLeaf(block_info.hash); if (removal_res.has_error()
  //          and removal_res
  //                  != outcome::failure(
  //                      blockchain::BlockTreeError::BLOCK_IS_NOT_LEAF)) {
  //        SL_WARN(log_,
  //                "Rolling back of block {} is failed: {}",
  //                block_info,
  //                removal_res.error());
  //      }
  //      return BabeError::CAN_NOT_SAVE_BLOCK;
  //    }
  //
  //    changes_tracker->onBlockAdded(
  //        block_info.hash, storage_sub_engine_, chain_sub_engine_);
  //
  //    telemetry_->notifyBlockImported(block_info,
  //    telemetry::BlockOrigin::kOwn); telemetry_->pushBlockStats();
  //
  //    // observe digest of block
  //    // (must be done strictly after block will be added)
  //    auto digest_tracking_res = digest_tracker_->onDigest(
  //        {.block_info = block_info, .header = block.header},
  //        block.header.digest);
  //
  //    if (digest_tracking_res.has_error()) {
  //      SL_WARN(log_,
  //              "Error while tracking digest of block {}: {}",
  //              block_info,
  //              digest_tracking_res.error());
  //      return outcome::success();
  //    }
  //
  //    // finally, broadcast the sealed block
  //    announce_transmitter_->blockAnnounce(network::BlockAnnounce{
  //        block.header,
  //        block_info == block_tree_->bestBlock() ? network::BlockState::Best
  //                                               :
  //                                               network::BlockState::Normal,
  //        common::Buffer{},
  //    });
  //    SL_DEBUG(
  //        log_,
  //        "Announced block number {} in slot {} (epoch {}) with timestamp {}",
  //        block.header.number,
  //        ctx.slot,
  //        ctx.epoch,
  //        now);
  //
  //    auto current_best_block = block_tree_->bestBlock();
  //
  //    // Create new offchain worker for block if it is best only
  //    if (current_best_block.number > previous_best_block.number) {
  //      auto ocw_res = offchain_worker_api_->offchain_worker(
  //          block.header.parent_hash, block.header);
  //      if (ocw_res.has_failure()) {
  //        log_->error("Can't spawn offchain worker for block {}: {}",
  //                    block_info,
  //                    ocw_res.error());
  //      }
  //    }
  //
  //    return outcome::success();
  //  }

  void Sassafras::changeLotteryEpoch(const Context &ctx,
                                     const EpochNumber &epoch,
                                     primitives::AuthorityIndex authority_index,
                                     const Epoch &sassafras_config) const {
    BOOST_ASSERT(ctx.keypair != nullptr);

    Threshold ticket_threshold =
        ticket_id_threshold(sassafras_config.config.redundancy_factor,
                            sassafras_config.epoch_length,
                            sassafras_config.config.attempts_number,
                            sassafras_config.authorities.size())
            .number;

    Threshold threshold;  // FIXME
    //        = calculateThreshold(sassafras_config.leadership_rate,
    //                                        sassafras_config.authorities,
    //                                        authority_index);

    lottery_->changeEpoch(epoch,
                          sassafras_config.randomness,
                          ticket_threshold,
                          threshold,
                          *ctx.keypair);
  }

}  // namespace kagome::consensus::sassafras
