/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>

#include "consensus/sassafras/types/randomness.hpp"
#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/bandersnatch/vrf.hpp"
#include "crypto/bandersnatch_types.hpp"
#include "primitives/transcript.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {

  struct TicketBody;

  using namespace kagome::crypto::bandersnatch;

  /// VRF input to claim slot ownership during block production.
  vrf::VrfInput slot_claim_input(const Randomness &randomness,
                                 SlotNumber slot,
                                 EpochNumber epoch);

  /// Signing-data to claim slot ownership during block production.
  vrf::VrfSignData slot_claim_sign_data(const Randomness &randomness,
                                        SlotNumber slot,
                                        EpochNumber epoch);

  /// VRF input to generate the ticket id.
  vrf::VrfInput ticket_id_input(const Randomness &randomness,
                                AttemptsNumber attempt,
                                EpochNumber epoch);

  /// VRF output to generate the ticket id.
  vrf::VrfOutput ticket_id_output(
      const crypto::BandersnatchSecretKey &secret_key,
      const vrf::VrfInput &input);

  /// VRF input to generate the revealed key.
  vrf::VrfInput revealed_key_input(const Randomness &randomness,
                                   AttemptsNumber attempt,
                                   EpochNumber epoch);

  /// Data to be signed via ring-vrf.
  vrf::VrfSignData ticket_body_sign_data(const TicketBody &ticket_body,
                                         vrf::VrfInput ticket_id_input);

  common::Blob<32> sign_data_challenge(const vrf::VrfSignData &sign_data);

  /// Make ticket-id from the given VRF input and output.
  ///
  /// Input should have been obtained via [`ticket_id_input`].
  /// Output should have been obtained from the input directly using the vrf
  /// secret key or from the vrf signature outputs.
  common::Blob<16> make_ticket_id(const vrf::VrfInput &input,
                                  const vrf::VrfOutput &output);

  /// Make revealed key seed from a given VRF input and ouput.
  ///
  /// Input should have been obtained via [`revealed_key_input`].
  /// Output should have been obtained from the input directly using the vrf
  /// secret key or from the vrf signature outputs.
  common::Blob<32> make_revealed_key_seed(const vrf::VrfInput &input,
                                          const vrf::VrfOutput &output);

}  // namespace kagome::consensus::sassafras
