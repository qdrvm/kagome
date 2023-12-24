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

namespace kagome::consensus::sassafras::vrf {

  using namespace crypto::bandersnatch::vrf;

  void accepts_range(std::ranges::view auto arg);

  using primitives::Transcript;

  //  constexpr auto operator""_bytes(const char *s, std::size_t size) {
  //    return std::string_view(s, size)
  //         | std::ranges::views::transform(
  //               [](char c) { return static_cast<uint8_t>(c); });
  //  }

  using VrfPreOut = Tagged<common::Buffer, struct VrfPreOutTag>;

  /// VRF input and pre-output paired together, possibly unverified.
  ///
  ///
  struct VrfInOut {
    /// VRF input point
    VrfInput input;
    /// VRF pre-output point
    VrfPreOut preoutput;

    /// VRF output reader via the supplied transcript.
    ///
    /// You should domain seperate outputs using the transcript.

    void vrf_output(Transcript t, auto out) {
      t.append_message("VrfOutput"_bytes, std::span(preoutput));
      t.challenge_bytes(""_bytes, out);
    }
  };

  struct VrfOutput {
    SCALE_TIE(1);

    VrfPreOut preoutput;

    template <size_t N>
    std::array<uint8_t, N> make_bytes(common::BufferView context,
                                      const VrfInput &input) const {
      VrfInOut inout{
          .input = input,
          .preoutput = preoutput,
      };

      Transcript transcript;
      transcript.initialize(context);

      std::array<uint8_t, N> res;
      inout.vrf_output(transcript, res);

      return res;
    }
  };

  /// VRF input to claim slot ownership during block production.
  VrfInput slot_claim_input(const Randomness &randomness,
                            SlotNumber slot,
                            EpochNumber epoch) {
    auto slot_bytes = common::uint64_to_le_bytes(slot);
    auto epoch_bytes = common::uint64_to_le_bytes(epoch);
    std::vector<libp2p::BytesIn> data{randomness, slot_bytes, epoch_bytes};

    return vrf_input_from_data("sassafras-claim-v1.0"_bytes, std::span(data));
  }

  /// Signing-data to claim slot ownership during block production.
  VrfSignData slot_claim_sign_data(const Randomness &randomness,
                                   SlotNumber slot,
                                   EpochNumber epoch) {
    static const auto transcript_label =
        "sassafras-slot-claim-transcript-v1.0"_bytes;

    static const std::vector<std::span<const uint8_t>> transcript_data;

    auto input = slot_claim_input(randomness, slot, epoch);

    return bandersnatch_vrf_sign_data(transcript_label.data(),
                                      transcript_label.size(),
                                      nullptr,
                                      nullptr,
                                      0,
                                      &input,
                                      1);
  }

  /// VRF input to generate the ticket id.
  VrfInput ticket_id_input(const Randomness &randomness,
                           AttemptsNumber attempt,
                           EpochNumber epoch) {
    auto attempt_bytes = common::uint64_to_le_bytes(attempt);
    auto epoch_bytes = common::uint64_to_le_bytes(epoch);
    std::vector<std::span<const uint8_t>> data{
        randomness, attempt_bytes, epoch_bytes};

    return vrf_input_from_data("sassafras-ticket-v1.0"_bytes, std::span(data));
  }

  /// VRF input to generate the revealed key.
  VrfInput revealed_key_input(const Randomness &randomness,
                              AttemptsNumber attempt,
                              EpochNumber epoch) {
    auto attempt_bytes = common::uint64_to_le_bytes(attempt);
    auto epoch_bytes = common::uint64_to_le_bytes(epoch);
    std::vector<std::span<const uint8_t>> data{
        randomness, attempt_bytes, epoch_bytes};

    return vrf_input_from_data("sassafras-revealed-v1.0"_bytes,
                               std::span(data));
  }

  /// Data to be signed via ring-vrf.
  VrfSignData ticket_body_sign_data(const TicketBody &ticket_body,
                                    VrfInput ticket_id_input) {
    static const auto transcript_label =
        "sassafras-ticket-body-transcript-v1.0"_bytes;

    auto encoded_ticket_body = scale::encode(ticket_body).value();
    std::vector<libp2p::BytesIn> transcript_data{encoded_ticket_body};

    std::vector<VrfInput> inputs{ticket_id_input};

    return vrf_sign_data(transcript_label, transcript_data, inputs);
  }

  /// Make ticket-id from the given VRF input and output.
  ///
  /// Input should have been obtained via [`ticket_id_input`].
  /// Output should have been obtained from the input directly using the vrf
  /// secret key or from the vrf signature outputs.
  TicketId make_ticket_id(const VrfInput &input, const VrfOutput &output) {
    auto bytes = output.make_bytes<16>("ticket-id"_bytes, input);
    return TicketId(common::le_bytes_to_uint128(bytes));
  }

  /// Make revealed key seed from a given VRF input and ouput.
  ///
  /// Input should have been obtained via [`revealed_key_input`].
  /// Output should have been obtained from the input directly using the vrf
  /// secret key or from the vrf signature outputs.
  common::Buffer make_revealed_key_seed(const VrfInput &input,
                                        const VrfOutput &output) {
    return common::Buffer(output.make_bytes<32>("revealed-seed"_bytes, input));
  }

}  // namespace kagome::consensus::sassafras::vrf
