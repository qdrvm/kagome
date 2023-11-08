/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/sassafras/impl/sassafras_lottery_impl.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/int_serialization.hpp"
#include "consensus/sassafras/impl/prepare_transcript.hpp"
#include "consensus/sassafras/impl/sassafras_vrf.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/sassafras/vrf.hpp"
#include "crypto/bandersnatch_provider.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/random_generator.hpp"
#include "crypto/vrf_provider.hpp"
#include "offchain/impl/runner.hpp"
#include "offchain/offchain_worker_factory.hpp"
#include "runtime/runtime_api/sassafras_api.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::consensus::sassafras {
  using common::Buffer;
  namespace vrf_constants = crypto::constants::sr25519::vrf;

  SassafrasLotteryImpl::SassafrasLotteryImpl(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::CSPRNG> random_generator,
      std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
      std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::SassafrasApi> api,
      std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory,
      std::shared_ptr<offchain::Runner> ocw_runner,
      std::shared_ptr<storage::SpacedStorage> storage)
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
        storage_{storage->getSpace(storage::Space::kDefault)} {
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
    epoch_ = std::numeric_limits<EpochNumber>::max();

    app_state_manager.atPrepare([this] { return prepare(); });
  }

  bool SassafrasLotteryImpl::prepare() {
    using LotteryData = std::tuple<decltype(epoch_),
                                   decltype(ticket_ids_),
                                   decltype(tickets_),
                                   decltype(next_ticket_ids_),
                                   decltype(next_tickets_)>;

    auto data_res = storage_->tryGet(storage::kSassafrasLotteryStateLookupKey);
    if (data_res.has_error()) {
      SL_WARN(
          logger_, "Failed to fetch saved lottery state: {}", data_res.error());
    } else if (data_res.value().has_value()) {
      auto decode_res = scale::decode<LotteryData>(data_res.value().value());
      if (decode_res.has_value()) {
        SL_WARN(logger_,
                "Failed to decode saved lottery state: {}",
                data_res.error());
      } else {
        std::tie(
            epoch_, ticket_ids_, tickets_, next_ticket_ids_, next_tickets_) =
            std::move(decode_res.value());
      }
    }

    return true;
  }

  void SassafrasLotteryImpl::changeEpoch(
      EpochNumber epoch,
      const Randomness &randomness,
      const Threshold &ticket_threshold,
      const Threshold &threshold,
      const crypto::BandersnatchKeypair &keypair,
      AttemptsNumber attempts) {
    SL_TRACE(logger_,
             "Epoch changed "
             "FROM epoch {} with randomness {} TO epoch {} with randomness {}",
             epoch_,
             randomness_,
             epoch,
             randomness);

    ticket_ids_.clear();
    if (epoch_ + 1 == epoch and next_ticket_ids_.has_value()) {
      ticket_ids_.swap(next_ticket_ids_.value());
    }
    next_ticket_ids_.reset();

    tickets_.clear();
    if (epoch_ + 1 == epoch and next_tickets_.has_value()) {
      tickets_.swap(next_tickets_.value());
    }
    next_tickets_.reset();

    epoch_ = epoch;
    randomness_ = randomness;
    ticket_threshold_ = ticket_threshold;
    threshold_ = threshold;
    keypair_ = keypair;
    attempts_ = attempts;

    // Try to load tickets from storage
    // If it has not loaded
    //   - generate
    generateTickets();  // await
    // For each
    //   - send next one
    //   - save tickets list and index last sent
  }

  EpochNumber SassafrasLotteryImpl::getEpoch() const {
    return epoch_;
  }

  void SassafrasLotteryImpl::generateTickets() {
    if (next_tickets_.has_value()) {
      // Already generated
      return;
    }

    auto next_epoch = epoch_ + 1;

    auto &tickets = next_tickets_.emplace();
    tickets.reserve(attempts_);

    for (AttemptsNumber attempt = 0; attempt < attempts_; ++attempt) {
      auto b1 = Buffer().put(next_randomness_);
      auto b2 = Buffer().putUint64(next_epoch);
      auto b3 = Buffer().putUint32(attempt);
      std::vector<std::span<uint8_t>> b{
          std::span(b1), std::span(b2), std::span(b3)};

      // --- Ticket Identifier Value ---

      auto ticket_id_vrf_input =
          vrf::ticket_id_input(next_randomness_, attempt, next_epoch);

      auto ticket_id_vrf_output =
          vrf_output(keypair_.secret_key, ticket_id_vrf_input);

      auto ticket_bytes =
          vrf_bytes<16>(ticket_id_vrf_input, ticket_id_vrf_output);

      auto ticket_id = TicketId(common::le_bytes_to_uint128(ticket_bytes));

      // --- Tickets Threshold ---

      // Check ticket id threshold
      if (ticket_id.number > ticket_threshold_) {
        continue;
      }

      // --- Ticket Body ---

      // erased key

      crypto::Ed25519Seed seed;
      random_generator_->fillRandomly(seed);
      auto erased_keypair =
          ed25519_provider_->generateKeypair(seed, {}).value();

      // revealed key

      std::string revealed_label = "sassafras-revealed-v1.0";
      auto revealed_vrf_input = vrf_input_from_items(
          std::span(reinterpret_cast<uint8_t *>(revealed_label.data()),
                    revealed_label.size()),
          b);

      auto revealed_vrf_output =
          vrf_output(keypair_.secret_key, revealed_vrf_input);

      auto revealed_seed_ =
          vrf_bytes<32>(revealed_vrf_input, revealed_vrf_output);

      auto revealed_seed =
          crypto::Ed25519Seed::fromSpan(revealed_seed_).value();

      auto revealed_keypair =
          ed25519_provider_->generateKeypair(revealed_seed, {}).value();

      // ticket body

      TicketBody ticket_body{.attempt_index = attempt,
                             .erased_public = erased_keypair.public_key,
                             .revealed_public = revealed_keypair.public_key};

      // --- Ring Signature Production ---

      std::string ticket_body_label = "sassafras-ticket-body-v1.0";
      auto encoded_ticket_body = scale::encode(ticket_body).value();
      std::vector<std::span<uint8_t>> transcript_data{encoded_ticket_body};
      std::vector<VrfInput> vrf_inputs{ticket_id_vrf_input};

      auto sign_data = vrf_signature_data(
          std::span(reinterpret_cast<uint8_t *>(ticket_body_label.data()),
                    ticket_body_label.size()),
          transcript_data,
          vrf_inputs);

      RingProver ring_prover;  // FIXME

      auto ring_signature =
          ring_vrf_sign(keypair_.secret_key, sign_data, ring_prover);

      // --- Ticket envelope ---

      //      TicketEnvelope ticket_envelope{
      //          .body = ticket_body,
      //          .signature = ring_signature,
      //      };

      tickets.emplace_back(ticket_body, ring_signature);
    }

    // Submit tickets over offchain worker

    auto encode_res = scale::encode(
        epoch_, ticket_ids_, tickets_, next_ticket_ids_, next_tickets_);
    if (encode_res.has_value()) {
      auto save_res = storage_->put(storage::kSassafrasLotteryStateLookupKey,
                                    Buffer{std::move(encode_res.value())});
      [[unlikely]] if (save_res.has_error()) {
        SL_WARN(logger_, "Failed to save lottery state: {}", save_res.error());
        next_ticket_ids_.reset();
        next_tickets_.reset();
        return;
      }
    } else {
      SL_WARN(logger_,
              "Failed to encode lottery state for save: {}",
              encode_res.error());
      next_ticket_ids_.reset();
      next_tickets_.reset();
      return;
    }

    auto label = fmt::format("tickets.{}", epoch_ + 1);

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
  }

  std::optional<crypto::VRFOutput> SassafrasLotteryImpl::getSlotLeadership(
      const primitives::BlockHash &block,
      SlotNumber slot,
      bool allow_fallback) const {
    BOOST_ASSERT_MSG(epoch_ != std::numeric_limits<uint64_t>::max(),
                     "Epoch must be initialized before this point");

    // Get ticket assigned with slot
    auto ticket_id_res = api_->slot_ticket_id(block, slot);
    if (ticket_id_res.has_error()) {
      SL_WARN(
          logger_, "Can't get ticket id for a slot: {}", ticket_id_res.error());
      return std::nullopt;
    }
    const auto &ticket_id_opt = ticket_id_res.value();

    // Ticket has gotten
    if (not ticket_id_opt.has_value()) {
      if (allow_fallback) {
        SL_DEBUG(logger_, "No ticket for a slot");
        // TODO(xDimon): Produce block by fallback way
      } else {
        SL_DEBUG(logger_, "No ticket for a slot, but leader by fallback way");
      }
      return std::nullopt;
    }
    const auto &ticket_id = ticket_id_opt.value();

    // Check if it is our ticket
    if (std::find(ticket_ids_.begin(), ticket_ids_.end(), ticket_id)
        == ticket_ids_.end()) {
      SL_DEBUG(logger_, "Slot is assigned with non-our ticket");
      return std::nullopt;
    }

    auto ticket_res = api_->slot_ticket(block, slot);
    if (ticket_res.has_error()) {
      SL_WARN(logger_, "Can't get ticket for a slot: {}", ticket_res.error());
      return std::nullopt;
    }
    BOOST_ASSERT_MSG(
        ticket_res.value().has_value()
            or std::get<0>(ticket_res.value().value()) != ticket_id,
        "Ticket must match with ticket id");
    const auto &ticket = ticket_res.value().value();

    const auto &ticket_body = std::get<1>(ticket);

    // --- Primary Claim Method ---

    auto b11 = Buffer().put(randomness_);
    auto b12 = Buffer().putUint64(epoch_);
    auto b13 = Buffer().putUint32(slot);
    std::vector<std::span<uint8_t>> b1{
        std::span(b11), std::span(b12), std::span(b13)};

    std::string randomness_label = "sassafras-randomness-v1.0";
    auto randomness_vrf_input = vrf_input_from_items(
        std::span(reinterpret_cast<uint8_t *>(randomness_label.data()),
                  randomness_label.size()),
        b1);

    auto b21 = Buffer().put(randomness_);
    auto b22 = Buffer().putUint64(epoch_);
    auto b23 = Buffer().putUint32(ticket_body.attempt_index);
    std::vector<std::span<uint8_t>> b2{
        std::span(b21), std::span(b22), std::span(b23)};

    std::string revealed_label = "sassafras-revealed-v1.0";
    auto revealed_vrf_input = vrf_input_from_items(
        std::span(reinterpret_cast<uint8_t *>(revealed_label.data()),
                  revealed_label.size()),
        b2);

    auto encoded_ticket_body = scale::encode(ticket_body).value();
    std::vector<std::span<uint8_t>> transcript_data{encoded_ticket_body};
    std::vector<VrfInput> vrf_inputs{randomness_vrf_input, revealed_vrf_input};

    std::string claim_label = "sassafras-claim-v1.0";
    auto sign_data = vrf_signature_data(
        std::span(reinterpret_cast<uint8_t *>(claim_label.data()),
                  claim_label.size()),
        transcript_data,
        vrf_inputs);

    // --- Secondary Claim Method ---

    std::vector<std::span<uint8_t>> td{};
    std::vector<VrfInput> vrfi{randomness_vrf_input};

    std::string claim_label_2 = "sassafras-slot-claim-transcript-v1.0";

    sign_data = vrf_signature_data(
        std::span(reinterpret_cast<uint8_t *>(claim_label_2.data()),
                  claim_label_2.size()),
        td,
        vrfi);

    //  --- old ---

    primitives::Transcript transcript;
    prepareTranscript(transcript, randomness_, slot, epoch_);

    // auto res =
    //     vrf_provider_->signTranscript(transcript, keypair_, threshold_);
    //
    // SL_TRACE(
    //     logger_,
    //     "prepareTranscript (leadership): randomness {}, slot {}, epoch {}{}",
    //     randomness_,
    //     slot,
    //     epoch_,
    //     res.has_value() ? " - SLOT LEADER" : "");
    //
    // return res;
    return std::nullopt;
  }

  crypto::VRFOutput SassafrasLotteryImpl::slotVrfSignature(
      SlotNumber slot) const {
    BOOST_ASSERT_MSG(epoch_ != std::numeric_limits<uint64_t>::max(),
                     "Epoch must be initialized before this point");

    primitives::Transcript transcript;
    prepareTranscript(transcript, randomness_, slot, epoch_);

    // auto res = vrf_provider_->signTranscript(transcript, keypair_);
    //
    // BOOST_ASSERT(res);
    // return res.value();
    return {};
  }

  std::optional<primitives::AuthorityIndex>
  SassafrasLotteryImpl::secondarySlotAuthor(
      SlotNumber slot,
      primitives::AuthorityListSize authorities_count,
      const Randomness &randomness) const {
    if (0 == authorities_count) {
      return std::nullopt;
    }

    auto rand = hasher_->blake2b_256(
        scale::encode(std::tuple(randomness, slot)).value());

    auto rand_number = common::be_bytes_to_uint256(rand);

    auto index = (rand_number % authorities_count)
                     .convert_to<primitives::AuthorityIndex>();

    SL_TRACE(logger_,
             "Secondary author for slot {} has index {}. "
             "({} authorities. Randomness: {})",
             slot,
             index,
             authorities_count,
             randomness);
    return index;
  }

}  // namespace kagome::consensus::sassafras
