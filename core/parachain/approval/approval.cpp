/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

extern "C" {
#include <schnorrkel/schnorrkel.h>
}

#include "consensus/babe/impl/prepare_transcript.hpp"
#include "parachain/approval/approval.hpp"
#include "primitives/transcript.hpp"

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

  outcome::result<void> UnsafeVRFOutput::compute_randomness(
      ::RelayVRFStory &vrf_story,
      const consensus::babe::Authorities &authorities,
      const consensus::Randomness &randomness,
      consensus::EpochNumber epoch_index) {
    if (authorities.size() <= authority_index) {
      return Error::AuthorityOutOfBounds;
    }

    const auto &author = authorities[authority_index].id;
    OUTCOME_TRY(pubkey, parachain::PublicKey::fromSpan(author));

    primitives::Transcript transcript;
    consensus::babe::prepareTranscript(
        transcript, randomness, slot, epoch_index);

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast)
    if (Sr25519SignatureResult::SR25519_SIGNATURE_RESULT_OK
        != sr25519_vrf_compute_randomness(
            pubkey.data(),
            const_cast<Strobe128 *>(
                reinterpret_cast<const Strobe128 *>(transcript.data().data())),
            reinterpret_cast<VRFCOutput *>(&vrf_output.get().output),
            &vrf_story)) {
      return Error::ComputeRandomnessFailed;
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast)

    return outcome::success();
  }

}  // namespace kagome::parachain::approval
