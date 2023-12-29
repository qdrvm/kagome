/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/types.hpp>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/bandersnatch_types.hpp"

struct bandersnatch_VrfInput;
struct bandersnatch_VrfOutput;
struct bandersnatch_VrfSignData;
struct bandersnatch_VrfSignature;
struct bandersnatch_RingProver;
struct bandersnatch_RingVerifier;
struct bandersnatch_RingVrfSignature;

namespace kagome::crypto::bandersnatch::vrf {

  using libp2p::Bytes;
  using libp2p::BytesIn;
  using libp2p::BytesOut;

  using VrfInput = ::bandersnatch_VrfInput *;
  using VrfOutput = common::Blob<BANDERSNATCH_PREOUT_SIZE>;
  using VrfSignData = ::bandersnatch_VrfSignData *;
  using RingProver = const ::bandersnatch_RingProver *;
  using RingVerifier = const ::bandersnatch_RingVerifier *;
  using RingSignature = common::Blob<BANDERSNATCH_RING_SIGNATURE_SIZE>;
  using Signature = common::Blob<BANDERSNATCH_SIGNATURE_SIZE>;

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

  /// VRF signature.
  ///
  /// Includes both the transcript `signature` and the `outputs` generated from
  struct VrfSignature {
    // SCALE_TIE(2);

    /// VRF (pre)outputs.
    VrfIosVec<crypto::bandersnatch::vrf::VrfOutput> outputs;

    /// Transcript signature.
    Signature signature;

    friend scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                                 const VrfSignature &x) {
      s << x.outputs;
      s << x.signature;
      return s;
    }

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 VrfSignature &x) {
      s >> x.outputs;
      s >> x.signature;
      return s;
    }
  };

  /// Ring VRF signature.
  struct RingVrfSignature {
    // SCALE_TIE(2);

    /// VRF (pre)outputs.
    VrfIosVec<VrfOutput> outputs;

    /// Ring signature.
    RingSignature signature;

    friend scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                                 const RingVrfSignature &x) {
      s << x.outputs;
      s << x.signature;
      return s;
    }

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 RingVrfSignature &x) {
      s >> x.outputs;
      s >> x.signature;
      return s;
    }
  };

  VrfInput vrf_input(BytesIn domain, BytesIn data);

  VrfInput vrf_input_from_data(BytesIn domain, std::span<BytesIn> data);

  VrfOutput vrf_output(BandersnatchSecretKey secret, VrfInput input);

  template <size_t N>
    requires(N == 16 or N == 32)
  common::Blob<N> make_bytes(BytesIn context, VrfInput input, VrfOutput output);

  VrfSignData vrf_sign_data(BytesIn label,
                            std::span<BytesIn> data,
                            std::span<VrfInput> inputs);

  template <size_t N>
    requires(N == 16 or N == 32)
  common::Blob<N> vrf_sign_data_challenge(vrf::VrfSignData sign_data);

  VrfSignature vrf_sign(BandersnatchSecretKey secret_key,
                        VrfSignData sign_data);

  bool vrf_verify(const VrfSignature &signature,
                  VrfSignData sign_data,
                  const BandersnatchPublicKey &public_key);

  RingVrfSignature ring_vrf_sign(BandersnatchSecretKey secret_key,
                                 VrfSignData sign_data,
                                 RingProver ring_prover);

  bool ring_vrf_verify(const RingVrfSignature &signature,
                       VrfSignData sign_data,
                       RingVerifier verifier);

}  // namespace kagome::crypto::bandersnatch::vrf
