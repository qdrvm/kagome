/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <schnorrkel/schnorrkel.h>

#include "parachain/approval/approval.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::approval,
                            UnsafeVRFOutput::Error,
                            e) {
  using E = kagome::parachain::approval::UnsafeVRFOutput::Error;
  switch (e) {
    case E::AuthorityOutOfBounds:
      return "Authority index out of bounds";
    case E::ComputeRandomnessFailed:
      return "Compute randomness failed";
  }
  return "Unknown UnsafeVRFOutput error";
}

namespace kagome::parachain::approval {

  outcome::result<RelayVRFStory> UnsafeVRFOutput::compute_randomness(
      const primitives::AuthorityList &authorities,
      const consensus::babe::Randomness &randomness,
      consensus::babe::EpochNumber epoch_index) {
    if (authorities.size() <= authority_index) {
      return Error::AuthorityOutOfBounds;
    }

    const auto &author = authorities[authority_index].id.id;
    OUTCOME_TRY(pubkey, parachain::PublicKey::fromSpan(author));

    primitives::Transcript transcript;
    consensus::babe::prepareTranscript(transcript, randomness, slot, epoch_index);

    ::RelayVRFStory vrf_story;
    if (Sr25519SignatureResult::SR25519_SIGNATURE_RESULT_OK
        != sr25519_vrf_compute_randomness(
            pubkey.data(),
            (Strobe128 *)(transcript.data().data()),  // NOLINT
            (VRFCOutput *)&vrf_output.get().output,
            &vrf_story)) {
      return Error::ComputeRandomnessFailed;
    }

    return approval::RelayVRFStory{
        .data = ConstBuffer::fromSpan(vrf_story.data).value()};
  }

}  // namespace kagome::parachain::approval
