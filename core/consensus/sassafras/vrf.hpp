/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>

#include "crypto/bandersnatch_types.hpp"
#include "primitives/transcript.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {

  using primitives::Transcript;

  using OCTET_STRING = std::span<uint8_t>;

  template <typename T>
  using SEQUENCE_OF = std::span<T>;

  using SEQUENCE_OF_OCTET_STRING = SEQUENCE_OF<OCTET_STRING>;

  // VRF Input ----------------

  struct VrfInput {
    SCALE_TIE(0);
    // opaque
  };

  inline VrfInput vrf_input(OCTET_STRING domain, OCTET_STRING buf) {
    return {};  // FIXME It's stub
  }

  inline VrfInput vrf_input_from_items(OCTET_STRING domain,
                                       SEQUENCE_OF_OCTET_STRING data) {
    common::Buffer buf;
    for (auto &item : data) {
      buf.put(item);
      buf.putUint8(item.size());
    }
    return vrf_input(domain, buf);
  }

  // VRF Output ----------------

  using VrfOutput =
      common::Blob<crypto::constants::bandersnatch::PREOUT_SERIALIZED_SIZE>;

  inline VrfOutput vrf_output(crypto::BandersnatchSecretKey secret,
                              VrfInput input) {
    return {};  // FIXME It's stub
  }

  template <size_t N>
  inline common::Blob<N> vrf_bytes(VrfInput vrf_input, VrfOutput vrf_output) {
    return {};  // FIXME It's stub
  }

  /// Max number of inputs/outputs which can be handled by the VRF signing
  /// procedures.
  ///
  /// The number is quite arbitrary and chosen to fulfill the use cases found so
  /// far. If required it can be extended in the future.
  constexpr size_t MAX_VRF_IOS = 3;

  template <typename T>
  concept VrfInputOrOutput =
      std::is_same_v<T, VrfInput> or std::is_same_v<T, VrfOutput>;

  template <typename T>
    requires VrfInputOrOutput<T>
  using VrfIosVec = common::SLVector<T, MAX_VRF_IOS>;

  // VRF Signature Data --------------

  struct VrfSignatureData {
    // represents a ark-transcript object.
    Transcript transcript;
    // sequence of VrfInputs to be signed.
    SEQUENCE_OF<VrfInput> vrf_input;
  };

  using TranscriptData = OCTET_STRING;

  Transcript TRANSCRIPT(OCTET_STRING transcript_label);  //

  inline VrfSignatureData vrf_signature_data(
      OCTET_STRING transcript_label,
      SEQUENCE_OF<TranscriptData> transcript_data,
      SEQUENCE_OF<VrfInput> vrf_inputs) {
    Transcript transcript;
    transcript.initialize(transcript_label);
    // for (auto &data : transcript_data) {
    //   // transcript.append(data);
    // }
    return VrfSignatureData{transcript, vrf_inputs};
  };

  // Plain VRF Signature -----------

  using Signature =
      common::Blob<crypto::constants::bandersnatch::SIGNATURE_SIZE>;

  /// VRF signature.
  ///
  /// Includes both the transcript `signature` and the `outputs` generated from
  /// the [`VrfSignData::inputs`].
  ///
  /// Refer to [`VrfSignData`] for more details.
  struct VrfSignature {
    SCALE_TIE(2);
    /// VRF (pre)outputs.
    VrfIosVec<VrfOutput> outputs;

    /// Transcript signature.
    Signature signature;

    //    friend scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream
    //    &s,
    //                                                 const VrfSignature &x) {
    //      return s          //
    //          << x.outputs  //
    //          << x.signature;
    //    }
    //
    //    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream
    //    &s,
    //                                                 VrfSignature &x) {
    //      return s          //
    //          >> x.outputs  //
    //          >> x.signature;
    //    }
  };

  VrfSignature plain_vrf_sign(crypto::BandersnatchSecretKey secret,
                              VrfSignatureData signature_data);

  // Ring VRF Signature -----------

  struct RingProof {
    SCALE_TIE(0);
    // opaque
  };
  struct RingProver {
    SCALE_TIE(0);
    // opaque
  };
  struct RingVerifier {
    SCALE_TIE(0);
    // opaque
  };

  using RingSignature = common::Blob<
      crypto::constants::bandersnatch::RING_SIGNATURE_SERIALIZED_SIZE>;

  /// Ring VRF signature.
  struct RingVrfSignature {
    SCALE_TIE(2);

    /// VRF (pre)outputs.
    VrfIosVec<VrfOutput> outputs;

    /// Ring signature.
    RingSignature signature;

    //    // represents the actual signature (opaque).
    //    // denotes the proof of ring membership.
    //    RingProof ring_proof;
    //    // sequence of VrfOutput objects corresponding to the VrfInput values.
    //    SEQUENCE_OF<VrfOutput> outputs;
  };

  inline RingVrfSignature ring_vrf_sign(crypto::BandersnatchSecretKey secret,
                                        VrfSignatureData signature_data,
                                        RingProver prover) {
    return {};  // FIXME It's stub
  }

  bool ring_vrf_verify(RingVrfSignature signature, RingVerifier verifier);

}  // namespace kagome::consensus::sassafras
