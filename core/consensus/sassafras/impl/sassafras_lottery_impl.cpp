/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/sassafras/impl/sassafras_lottery_impl.hpp"

#include "blockchain/block_tree.hpp"
#include "common/int_serialization.hpp"
#include "consensus/sassafras/impl/prepare_transcript.hpp"
#include "consensus/sassafras/impl/sassafras_vrf.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/sassafras/vrf.hpp"
#include "crypto/bandersnatch_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/random_generator.hpp"
#include "crypto/vrf_provider.hpp"
#include "runtime/runtime_api/sassafras_api.hpp"

namespace kagome::consensus::sassafras {
  using common::Buffer;
  namespace vrf_constants = crypto::constants::sr25519::vrf;

  SassafrasLotteryImpl::SassafrasLotteryImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::CSPRNG> random_generator,
      std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::SassafrasApi> api)
      : logger_{log::createLogger("SassafrasLottery", "sassafras_lottery")},
        block_tree_{std::move(block_tree)},
        random_generator_{std::move(random_generator)},
        bandersnatch_provider_{std::move(bandersnatch_provider)},
        vrf_provider_{std::move(vrf_provider)},
        hasher_{std::move(hasher)},
        api_{std::move(api)} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(random_generator_);
    BOOST_ASSERT(bandersnatch_provider_);
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(api_);
    epoch_ = std::numeric_limits<EpochNumber>::max();
  }

  void SassafrasLotteryImpl::changeEpoch(
      EpochNumber epoch,
      const Randomness &randomness,
      const Threshold &ticket_threshold,
      const Threshold &threshold,
      const crypto::BandersnatchKeypair &keypair) {
    SL_TRACE(logger_,
             "Epoch changed "
             "FROM epoch {} with randomness {} TO epoch {} with randomness {}",
             epoch_,
             randomness_,
             epoch,
             randomness);

    epoch_ = epoch;
    randomness_ = randomness;
    ticket_threshold_ = ticket_threshold;
    threshold_ = threshold;
    keypair_ = keypair;

    // Try to load tickets from storage
    // If it has not loaded
    //   - generate
    generateTickets();
    // For each
    //   - send next one
    //   - save tickets list and index last sent
  }

  EpochNumber SassafrasLotteryImpl::getEpoch() const {
    return epoch_;
  }

  void SassafrasLotteryImpl::generateTickets() {
    if (tickets_.epoch_for == epoch_ + 1) {
      // Already generated
      return;
    }

    tickets_.epoch_for = epoch_ + 1;
    tickets_.next_send_index = 0;

    uint32_t attempt_number = 1;  // FIXME It's stub. Need to take from config

    std::vector<TicketEnvelope> tickets;
    tickets.reserve(attempt_number);

    for (uint32_t attempt_index = 0; attempt_index < attempt_number;
         ++attempt_index) {
      auto b1 = Buffer().put(next_randomness_);
      auto b2 = Buffer().putUint64(next_epoch_);
      auto b3 = Buffer().putUint32(attempt_index);
      std::vector<std::span<uint8_t>> b{
          std::span(b1), std::span(b2), std::span(b3)};

      // --- Ticket Identifier Value ---

      auto ticket_id_vrf_input =
          vrf::ticket_id_input(next_randomness_, attempt_index, next_epoch_);

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

      crypto::BandersnatchSeed seed;
      random_generator_->fillRandomly(seed);
      crypto::BandersnatchKeypair erased_keypair =
          bandersnatch_provider_->generateKeypair(seed, {});

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
          crypto::BandersnatchSeed::fromSpan(revealed_seed_).value();

      auto revealed_keypair =
          bandersnatch_provider_->generateKeypair(revealed_seed, {});

      // ticket body

      TicketBody ticket_body{.attempt_index = attempt_index,
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

    auto submission_res = api_->submit_tickets_unsigned_extrinsic(
        block_tree_->bestBlock().hash, tickets);
    if (submission_res.has_error()) {
      SL_WARN(logger_,
              "Submission of ticket was failed: {}",
              submission_res.error());
    }
  }

  std::optional<crypto::VRFOutput> SassafrasLotteryImpl::getSlotLeadership(
      const primitives::BlockHash &block, SlotNumber slot) const {
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
      SL_DEBUG(logger_, "No ticket for a slot");
      return std::nullopt;
    }
    const auto &ticket_id = ticket_id_opt.value();

    // Check if it is our ticket
    auto it = std::find_if(
        tickets_.tickets.begin(), tickets_.tickets.end(), [&](const auto &p) {
          return std::get<1>(p) == ticket_id;
        });
    if (it == tickets_.tickets.end()) {
      SL_DEBUG(logger_, "Slot is assigned with non-our ticket");
      return std::nullopt;
    }

    auto ticket_res = api_->slot_ticket(block, slot);
    if (ticket_res.has_error()) {
      SL_WARN(logger_, "Can't get ticket for a slot: {}", ticket_res.error());
      return std::nullopt;
    }
    const auto &ticket_opt = ticket_res.value();
    if (not ticket_opt.has_value() or std::get<0>(*ticket_opt) != ticket_id) {
      SL_WARN(logger_, "Ticket does not match with ticket id");
      return std::nullopt;
    }
    const auto &ticket = ticket_opt.value();

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
