/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <climits>
#include <cstdint>
#include <ranges>
#include <span>

#include "common/buffer.hpp"
#include "common/int_serialization.hpp"
#include "common/tagged.hpp"
#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/bandersnatch/vrf.hpp"
#include "primitives/transcript.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {

  struct TicketBody;

  using namespace kagome::crypto::bandersnatch;

  /// VRF input to claim slot ownership during block production.
  vrf::VrfInput slot_claim_input(const Randomness &randomness,
                                 SlotNumber slot,
                                 EpochNumber epoch) {
    const auto slot_blob = common::uint32_to_be_bytes(slot);
    const auto epoch_blob = common::uint64_to_be_bytes(epoch);
    std::vector<vrf::BytesIn> data{randomness, slot_blob, epoch_blob};
    return vrf::vrf_input_from_data("sassafras-claim-v1.0"_bytes, data);
  }

  /// Signing-data to claim slot ownership during block production.
  vrf::VrfSignData slot_claim_sign_data(const Randomness &randomness,
                                        SlotNumber slot,
                                        EpochNumber epoch) {
    auto input = slot_claim_input(randomness, slot, epoch);
    std::vector<vrf::VrfInput> inputs{input};
    return vrf::vrf_sign_data(
        "sassafras-slot-claim-transcript-v1.0"_bytes, {}, inputs);
  }

  /// VRF input to generate the ticket id.
  vrf::VrfInput ticket_id_input(const Randomness &randomness,
                                AttemptsNumber attempt,
                                EpochNumber epoch) {
    const auto attempt_blob = common::uint32_to_be_bytes(attempt);
    const auto epoch_blob = common::uint64_to_be_bytes(epoch);
    std::vector<vrf::BytesIn> data{randomness, attempt_blob, epoch_blob};
    return vrf::vrf_input_from_data("sassafras-ticket-v1.0"_bytes, data);
  }

  /// VRF output to generate the ticket id.
  vrf::VrfOutput ticket_id_output(
      const crypto::BandersnatchSecretKey &secret_key, vrf::VrfInput input) {
    return vrf::vrf_output(secret_key, input);
  }

  /// VRF input to generate the revealed key.
  vrf::VrfInput revealed_key_input(const Randomness &randomness,
                                   AttemptsNumber attempt,
                                   EpochNumber epoch) {
    const auto attempt_blob = common::uint32_to_be_bytes(attempt);
    const auto epoch_blob = common::uint64_to_be_bytes(epoch);
    std::vector<vrf::BytesIn> data{randomness, attempt_blob, epoch_blob};
    return vrf::vrf_input_from_data("sassafras-revealed-v1.0"_bytes, data);
  }

  /// Data to be signed via ring-vrf.
  vrf::VrfSignData ticket_body_sign_data(const TicketBody &ticket_body,
                                         vrf::VrfInput ticket_id_input) {
    auto encoded_ticket_body = scale::encode(ticket_body).value();
    std::vector<vrf::BytesIn> transcript_data{encoded_ticket_body};
    std::vector<vrf::VrfInput> inputs{ticket_id_input};
    return vrf::vrf_sign_data(
        "sassafras-ticket-body-transcript-v1.0"_bytes, transcript_data, inputs);
  }

  common::Blob<32> sign_data_challenge(vrf::VrfSignData sign_data) {
    auto bytes = vrf::vrf_sign_data_challenge<32>(sign_data);
    return bytes;
  }

  /// Make ticket-id from the given VRF input and output.
  ///
  /// Input should have been obtained via [`ticket_id_input`].
  /// Output should have been obtained from the input directly using the vrf
  /// secret key or from the vrf signature outputs.
  common::Blob<16> make_ticket_id(vrf::VrfInput input, vrf::VrfOutput output) {
    auto bytes = vrf::make_bytes<16>("ticket-id"_bytes, input, output);
    return bytes;
    // return TicketId(common::le_bytes_to_uint128(bytes));
  }

  /// Make revealed key seed from a given VRF input and ouput.
  ///
  /// Input should have been obtained via [`revealed_key_input`].
  /// Output should have been obtained from the input directly using the vrf
  /// secret key or from the vrf signature outputs.
  common::Blob<32> make_revealed_key_seed(vrf::VrfInput input,
                                          vrf::VrfOutput output) {
    auto bytes = vrf::make_bytes<32>("revealed-seed"_bytes, input, output);
    return bytes;
  }

}  // namespace kagome::consensus::sassafras
