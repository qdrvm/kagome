/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/sassafras/impl/sassafras_lottery_impl.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/int_serialization.hpp"
#include "consensus/sassafras/bandersnatch.hpp"
#include "consensus/sassafras/impl/prepare_transcript.hpp"
#include "consensus/sassafras/impl/sassafras_vrf.hpp"
#include "consensus/sassafras/sassafras_config_repository.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/timeline/impl/slot_leadership_error.hpp"
#include "crypto/bandersnatch_provider.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/key_store/session_keys.hpp"
#include "crypto/random_generator.hpp"
#include "crypto/vrf_provider.hpp"
#include "offchain/impl/runner.hpp"
#include "offchain/offchain_worker_factory.hpp"
#include "runtime/runtime_api/sassafras_api.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"
#include "threshold_util.hpp"

namespace kagome::consensus::sassafras {
  using common::Buffer;
  namespace vrf_constants = crypto::constants::sr25519::vrf;

  SassafrasLotteryImpl::SassafrasLotteryImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::CSPRNG> random_generator,
      std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
      std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::SassafrasApi> api,
      std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory,
      std::shared_ptr<offchain::Runner> ocw_runner,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<SassafrasConfigRepository> sassafras_config_repo,
      std::shared_ptr<crypto::SessionKeys> session_keys)
      : logger_{log::createLogger("SassafrasLottery", "sassafras_lottery")},
        block_tree_{std::move(block_tree)},
        random_generator_{std::move(random_generator)},
        bandersnatch_provider_{std::move(bandersnatch_provider)},
        ed25519_provider_{std::move(ed25519_provider)},
        vrf_provider_{std::move(vrf_provider)},
        hasher_{std::move(hasher)},
        api_{std::move(api)},
        ocw_factory_{std::move(ocw_factory)},
        ocw_runner_{std::move(ocw_runner)},
        storage_{storage->getSpace(storage::Space::kDefault)},
        sassafras_config_repo_{std::move(sassafras_config_repo)},
        session_keys_{std::move(session_keys)} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(random_generator_);
    BOOST_ASSERT(bandersnatch_provider_);
    BOOST_ASSERT(ed25519_provider_);
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(api_);
    BOOST_ASSERT(ocw_factory_);
    BOOST_ASSERT(ocw_runner_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(sassafras_config_repo_);
    BOOST_ASSERT(session_keys_);
    epoch_ = std::numeric_limits<EpochNumber>::max();
  }

  void SassafrasLotteryImpl::load() {
    auto data_res = storage_->tryGet(storage::kSassafrasLotteryStateLookupKey);
    if (data_res.has_error()) {
      SL_WARN(
          logger_, "Failed to fetch saved lottery state: {}", data_res.error());
      return;
    }

    using LotteryData = std::
        tuple<decltype(epoch_), decltype(tickets_), decltype(next_tickets_)>;

    if (not data_res.value().has_value()) {
      // No previously saved data
      return;
    }

    auto decode_res = scale::decode<LotteryData>(data_res.value().value());
    if (decode_res.has_value()) {
      std::tie(epoch_, tickets_, next_tickets_) = std::move(decode_res.value());
      return;
    }

    SL_WARN(logger_,
            "Failed to decode saved lottery state: {}",
            decode_res.error());
  }

  void SassafrasLotteryImpl::store() const {
    auto encode_res = scale::encode(epoch_, tickets_, next_tickets_);
    [[unlikely]] if (encode_res.has_error()) {
      SL_WARN(logger_,
              "Failed to encode lottery state for save: {}",
              encode_res.error());
      return;
    }

    auto save_res = storage_->put(storage::kSassafrasLotteryStateLookupKey,
                                  Buffer{std::move(encode_res.value())});
    if (save_res.has_value()) {
      return;
    }

    SL_WARN(logger_, "Failed to save lottery state: {}", save_res.error());
  }

  EpochNumber SassafrasLotteryImpl::getEpoch() const {
    return epoch_;
  }

  bool SassafrasLotteryImpl::changeEpoch(
      EpochNumber epoch, const primitives::BlockInfo &best_block) {
    // If is not initialized, then try to load pre-saved data
    if (epoch_ == std::numeric_limits<EpochNumber>::max()) {
      load();
    }

    if (epoch_ != epoch) {
      tickets_.clear();
      // Shift tickets if epoch changes to the next one
      if (epoch_ + 1 == epoch and next_tickets_.has_value()) {
        tickets_.swap(next_tickets_.value());
      }
      next_tickets_.reset();
    }

    // Setup state for actual epoch
    if (auto res = setupActualEpoch(epoch, best_block); res.has_error()) {
      SL_WARN(logger_, "Can't setup epoch {}: {}", epoch, res.error());
    }

    if (auto res = generateTickets(epoch + 1, best_block); res.has_error()) {
      SL_WARN(logger_,
              "Can't generate tickets for epoch {}: {}",
              epoch + 1,
              res.error());
    }

    SL_TRACE(logger_, "Epoch changed to epoch {}", epoch_);
    return keypair_.has_value();
  }

  outcome::result<void> SassafrasLotteryImpl::setupActualEpoch(
      EpochNumber epoch, const primitives::BlockInfo &best_block) {
    epoch_ = epoch;

    auto config_res = sassafras_config_repo_->config(best_block, epoch);
    [[unlikely]] if (config_res.has_error()) {
      SL_ERROR(logger_,
               "Can not get epoch: {}; Skipping slot processing",
               config_res.error());
      return config_res.as_failure();
    }
    auto &config = *config_res.value();

    auth_number_ = config.authorities.size();

    keypair_ = session_keys_->getSassafrasKeyPair(config.authorities);

    return outcome::success();
  }

  outcome::result<void> SassafrasLotteryImpl::generateTickets(
      EpochNumber epoch, const primitives::BlockInfo &best_block) {
    // Check if tickets for the next epoch are already generated
    if (next_tickets_.has_value()) {
      return outcome::success();
    }

    // Config of next epoch
    auto config_res = sassafras_config_repo_->config(best_block, epoch);
    [[unlikely]] if (config_res.has_error()) {
      SL_ERROR(logger_,
               "Can not get config for epoch {}: {}; Skip of ticket generating",
               epoch,
               config_res.error());
      return config_res.as_failure();
    }
    const auto &config = *config_res.value();
    const auto attempts_number = config.config.attempts_number;
    const auto &randomness = config.randomness;

    // Our actual keypair for the next epoch
    auto keypair = session_keys_->getSassafrasKeyPair(config.authorities);
    if (not keypair) {
      SL_VERBOSE(
          logger_,
          "Authorities are not match any our keys; Skip of ticket generating");
      return SlotLeadershipError::NO_VALIDATOR;
    }
    const auto &secret_key = keypair->first->secret_key;
    const auto authority_idx = keypair->second;

    // Ring context (for making prover)
    auto ring_context_res = api_->ring_context(best_block.hash);
    [[unlikely]] if (ring_context_res.has_error()) {
      SL_ERROR(
          logger_, "Unable to read ring context: {}", ring_context_res.error());
      return ring_context_res.as_failure();
    }
    if (not ring_context_res.value().has_value()) {
      SL_ERROR(logger_, "Ring context not initialized yet");
      return outcome::success();
    }
    auto &ring_context = ring_context_res.value().value();

    // Prover for making ring signature
    SL_TRACE(logger_, "Generating ring prover key...");
    auto ring_prover = ring_context.prover(config.authorities, authority_idx);
    SL_TRACE(logger_, "  ...done");

    // Threshold for filter tickets
    auto ticket_threshold = ticket_id_threshold(config.config.redundancy_factor,
                                                config.epoch_length,
                                                config.config.attempts_number,
                                                config.authorities.size());

    next_tickets_.emplace();
    next_tickets_->reserve(attempts_number);

    for (AttemptsNumber attempt = 0; attempt < attempts_number; ++attempt) {
      // --- Ticket Identifier Value ---

      // Make ticket id
      auto ticket_id_vrf_input = ticket_id_input(randomness, attempt, epoch);
      auto ticket_id_vrf_output =
          vrf::vrf_output(secret_key, ticket_id_vrf_input);
      auto ticket_bytes =
          make_ticket_id(ticket_id_vrf_input, ticket_id_vrf_output);
      auto ticket_id = TicketId(common::le_bytes_to_uint128(ticket_bytes));

      // Check ticket id threshold
      if (ticket_id.number > ticket_threshold.number) {
        continue;
      }

      // --- Ticket Body ---

      // erased key

      crypto::Ed25519Seed erased_seed;
      random_generator_->fillRandomly(erased_seed);
      auto erased_keypair =
          ed25519_provider_->generateKeypair(erased_seed, {}).value();

      // revealed key

      auto revealed_vrf_input = revealed_key_input(randomness, attempt, epoch);
      auto revealed_vrf_output =
          vrf::vrf_output(secret_key, revealed_vrf_input);
      auto revealed_seed_bytes =
          make_revealed_key_seed(revealed_vrf_input, revealed_vrf_output);
      auto revealed_seed =
          crypto::Ed25519Seed::fromSpan(revealed_seed_bytes).value();
      auto revealed_keypair =
          ed25519_provider_->generateKeypair(revealed_seed, {}).value();

      // ticket body

      TicketBody ticket_body{.attempt_index = attempt,
                             .erased_public = erased_keypair.public_key,
                             .revealed_public = revealed_keypair.public_key};

      // --- Ring Signature Production ---

      SL_DEBUG(logger_, ">>> Creating ring proof for attempt {}", attempt);
      auto sign_data = ticket_body_sign_data(ticket_body, ticket_id_vrf_input);

      auto ring_signature =
          vrf::ring_vrf_sign(secret_key, sign_data, ring_prover);
      SL_TRACE(logger_, "  ...done");

      BOOST_ASSERT(ticket_id_vrf_output == ring_signature.outputs[0]);

      // --- Ticket envelope ---

      TicketEnvelope ticket_envelope{
          .body = std::move(ticket_body),
          .signature = std::move(ring_signature),
      };

      next_tickets_->emplace_back(ticket_id, ticket_envelope, erased_seed);
    }

    // Save generated tickets

    store();

    // Submit tickets over offchain worker

    std::vector<TicketEnvelope> tickets;
    std::transform(next_tickets_->begin(),
                   next_tickets_->end(),
                   std::back_inserter(tickets),
                   [](const Ticket &ticket) { return ticket.envelope; });

    auto label = fmt::format("tickets.{}", epoch);

    auto func = [log = logger_,
                 api = api_,
                 block = block_tree_->bestBlock().hash,
                 tickets] {
      auto submission_res =
          api->submit_tickets_unsigned_extrinsic(block, tickets);
      if (submission_res.has_error()) {
        SL_WARN(log,
                "Submission of tickets was failed: {}",
                submission_res.error());
      }
    };

    ocw_runner_->run([worker = ocw_factory_->make(),
                      label = std::move(label),
                      func = std::move(func)] { worker->run(func, label); });

    return outcome::success();
  }

  std::optional<SlotLeadership> SassafrasLotteryImpl::getSlotLeadership(
      const primitives::BlockHash &block, SlotNumber slot) const {
    BOOST_ASSERT_MSG(epoch_ != std::numeric_limits<uint64_t>::max(),
                     "Epoch must be initialized before this point");
    BOOST_ASSERT_MSG(keypair_.has_value(), "Node must be active validator");

    // Get ticket assigned with slot
    auto ticket_id_res = api_->slot_ticket_id(block, slot);
    if (ticket_id_res.has_error()) {
      SL_WARN(
          logger_, "Can't get ticket id for a slot: {}", ticket_id_res.error());
      return std::nullopt;
    }
    const auto &ticket_id_opt = ticket_id_res.value();

    // No ticket for a slot - trying to use fallback way
    if (not ticket_id_opt.has_value()) {
      auto auth_index_of_leader = le_bytes_to_uint64(hasher_->blake2b_64(
                                      scale::encode(randomness_, slot).value()))
                                % auth_number_;

      if (keypair_->second == auth_index_of_leader) {
        return secondarySlotLeadership(slot);
      }

      return std::nullopt;
    }
    const auto &ticket_id = ticket_id_opt.value();

    // Check if it is our ticket

    auto it =
        std::find_if(tickets_.begin(), tickets_.end(), [&](const auto &ticket) {
          return ticket.id == ticket_id;
        });

    if (it != tickets_.end()) {
      const auto &ticket = *it;
      return primarySlotLeadership(slot, ticket);
    }

    SL_DEBUG(logger_, "Slot is assigned with non-our ticket");
    return std::nullopt;
  }

  SlotLeadership SassafrasLotteryImpl::primarySlotLeadership(
      SlotNumber slot, const Ticket &ticket) const {
    // --- Primary Claim Method ---

    const auto &ticket_body = ticket.envelope.body;

    auto encoded_ticket_body = scale::encode(ticket_body).value();
    std::vector<vrf::BytesIn> transcript_data{encoded_ticket_body};

    auto input_1 = slot_claim_input(randomness_, slot, epoch_);
    auto input_2 =
        revealed_key_input(randomness_, ticket_body.attempt_index, epoch_);
    std::vector<vrf::VrfInput> inputs{input_1, input_2};

    auto vrf_sign_data =  // consider to call slot_claim_sign_data()
        vrf::vrf_sign_data("sassafras-slot-claim-transcript-v1.0"_bytes,
                           transcript_data,
                           inputs);

    auto signature = vrf::vrf_sign(keypair_->first->secret_key, vrf_sign_data);

    // Sign some data using the erased key to enforce our ownership
    auto data = vrf::vrf_sign_data_challenge<32>(vrf_sign_data);

    auto erased_pair =
        ed25519_provider_->generateKeypair(ticket.erased_seed, {}).value();

    auto erased_signature = ed25519_provider_->sign(erased_pair, data).value();

    return SlotLeadership{.authority_index = keypair_->second,
                          .keypair = keypair_->first,
                          .signature = std::move(signature),
                          .ticket_claim = TicketClaim{erased_signature}};
  }

  SlotLeadership SassafrasLotteryImpl::secondarySlotLeadership(
      SlotNumber slot) const {
    // --- Secondary Claim Method ---

    std::vector<vrf::VrfInput> inputs{
        slot_claim_input(randomness_, slot, epoch_)};

    auto vrf_sign_data =  // consider to call slot_claim_sign_data()
        vrf::vrf_sign_data(
            "sassafras-slot-claim-transcript-v1.0"_bytes, {}, inputs);

    auto signature = vrf::vrf_sign(keypair_->first->secret_key, vrf_sign_data);

    return SlotLeadership{.authority_index = keypair_->second,
                          .keypair = keypair_->first,
                          .signature = std::move(signature),
                          .ticket_claim = std::nullopt};
  }

}  // namespace kagome::consensus::sassafras
