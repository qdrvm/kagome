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

  struct VrfOutput {
    SCALE_TIE(0);
    // opaque
  };

  inline VrfOutput vrf_output(crypto::BandersnatchSecretKey secret,
                              VrfInput input) {
    return {};  // FIXME It's stub
  }

  template <size_t N>
  inline std::array<uint8_t, N> vrf_bytes(VrfInput vrf_input,
                                          VrfOutput vrf_output) {
    return {};  // FIXME It's stub
  }

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
    //    for (auto &data : transcript_data) {
    //      // transcript.append(data);
    //    }
    return VrfSignatureData{transcript, vrf_inputs};
  };

  // Plain VRF Signature -----------

  using PlainSignature = OCTET_STRING;

  struct VrfSignature {
    SCALE_TIE(2);
    // represents the actual signature (opaque).
    PlainSignature signature;
    // a sequence of VrfOutputs corresponding to the VrfInputs values.
    SEQUENCE_OF<VrfOutput> outputs;
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

  using RingSignature = OCTET_STRING;

  struct RingVrfSignature {
    SCALE_TIE(3);

    // represents the actual signature (opaque).
    RingSignature signature;
    // denotes the proof of ring membership.
    RingProof ring_proof;
    // sequence of VrfOutput objects corresponding to the VrfInput values.
    SEQUENCE_OF<VrfOutput> outputs;
  };

  inline RingVrfSignature ring_vrf_sign(crypto::BandersnatchSecretKey secret,
                                        VrfSignatureData signature_data,
                                        RingProver prover) {
    return {};  // FIXME It's stub
  }

  bool ring_vrf_verify(RingVrfSignature signature, RingVerifier verifier);

}  // namespace kagome::consensus::sassafras
