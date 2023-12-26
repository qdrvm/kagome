/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>

#include "crypto/bandersnatch/vrf.hpp"
#include "crypto/bandersnatch_types.hpp"
#include "primitives/transcript.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {

  using primitives::Transcript;

  using OCTET_STRING = std::span<uint8_t>;

  template <typename T>
  using SEQUENCE_OF = std::span<T>;

  using SEQUENCE_OF_OCTET_STRING = SEQUENCE_OF<OCTET_STRING>;

  // VRF Output ----------------

  //  template <size_t N>
  //  inline common::Blob<N> vrf_bytes(VrfInput vrf_input, VrfOutput vrf_output)
  //  {
  //    return {};  // FIXME It's stub
  //  }

  /// Max number of inputs/outputs which can be handled by the VRF signing
  /// procedures.
  ///
  /// The number is quite arbitrary and chosen to fulfill the use cases found so
  /// far. If required it can be extended in the future.
  constexpr size_t MAX_VRF_IOS = 3;

  template <typename T>
  concept VrfInputOrOutput =
      std::is_same_v<T, crypto::bandersnatch::vrf::VrfInput>
      or std::is_same_v<T, crypto::bandersnatch::vrf::VrfOutput>;

  template <typename T>
    requires VrfInputOrOutput<T>
  using VrfIosVec = common::SLVector<T, MAX_VRF_IOS>;

  // VRF Signature Data --------------

  struct VrfSignatureData {
    // represents a ark-transcript object.
    Transcript transcript;
    // sequence of VrfInputs to be signed.
    SEQUENCE_OF<crypto::bandersnatch::vrf::VrfInput> vrf_input;
  };

  using TranscriptData = OCTET_STRING;

  Transcript TRANSCRIPT(OCTET_STRING transcript_label);  //

  inline VrfSignatureData vrf_signature_data(
      OCTET_STRING transcript_label,
      SEQUENCE_OF<TranscriptData> transcript_data,
      SEQUENCE_OF<crypto::bandersnatch::vrf::VrfInput> vrf_inputs) {
    Transcript transcript;
    transcript.initialize(transcript_label);
    // for (auto &data : transcript_data) {
    //   // transcript.append(data);
    // }
    return VrfSignatureData{transcript, vrf_inputs};
  };

  // Plain VRF Signature -----------

  using Signature = crypto::BandersnatchSignature;

  /// VRF signature.
  ///
  /// Includes both the transcript `signature` and the `outputs` generated from
  /// the [`VrfSignData::inputs`].
  ///
  /// Refer to [`VrfSignData`] for more details.
  struct VrfSignature {
    SCALE_TIE(2);
    /// VRF (pre)outputs.
    VrfIosVec<crypto::bandersnatch::vrf::VrfOutput> outputs;

    /// Transcript signature.
    Signature signature;

    friend scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                                 const VrfSignature &x) {
      return s  //
                //              << x.outputs  //
          << x.signature;
    }

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 VrfSignature &x) {
      return s  //
                //              >> x.outputs  //
          >> x.signature;
    }
  };

  VrfSignature plain_vrf_sign(crypto::BandersnatchSecretKey secret,
                              VrfSignatureData signature_data);

  // Ring VRF Signature -----------

  //  struct RingProof {
  //    SCALE_TIE(0);
  //    // opaque
  //  };
  //  struct RingVerifier {
  //    SCALE_TIE(0);
  //    // opaque
  //  };

  //  using RingSignature = common::Blob<
  //      crypto::constants::bandersnatch::RING_SIGNATURE_SERIALIZED_SIZE>;

  /// Ring VRF signature.
  struct RingVrfSignature {
    // SCALE_TIE(2);

    /// VRF (pre)outputs.
    VrfIosVec<crypto::bandersnatch::vrf::VrfOutput> outputs;

    /// Ring signature.
    crypto::bandersnatch::vrf::RingSignature signature;

    //    // represents the actual signature (opaque).
    //    // denotes the proof of ring membership.
    //    RingProof ring_proof;
    //    // sequence of VrfOutput objects corresponding to the VrfInput values.
    //    SEQUENCE_OF<VrfOutput> outputs;

    friend scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                                 const RingVrfSignature &x) {
      return s  //
                //              << x.outputs  //
          << x.signature;
    }

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 RingVrfSignature &x) {
      return s  //
                //              >> x.outputs  //
          >> x.signature;
    }
  };

}  // namespace kagome::consensus::sassafras
