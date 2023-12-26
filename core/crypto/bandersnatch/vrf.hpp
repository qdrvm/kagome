/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/types.hpp>

#include "common/buffer.hpp"
#include "crypto/bandersnatch_types.hpp"

struct bandersnatch_VrfInput;
struct bandersnatch_VrfOutput;
struct bandersnatch_VrfSignData;
struct bandersnatch_RingProver;
struct bandersnatch_RingVerifier;
struct bandersnatch_RingVrfSignature;

namespace kagome::crypto::bandersnatch::vrf {

  using libp2p::Bytes;
  using libp2p::BytesIn;
  using libp2p::BytesOut;

  using VrfInput = ::bandersnatch_VrfInput *;
  using VrfOutput = ::bandersnatch_VrfOutput *;
  using VrfSignData = ::bandersnatch_VrfSignData *;
  using RingProver = const ::bandersnatch_RingProver *;
  using RingVrifier = const ::bandersnatch_RingVerifier *;
  using RingVrfSignature = ::bandersnatch_RingVrfSignature *;
  using RingSignature = common::Blob<BANDERSNATCH_RING_SIGNATURE_SIZE>;

  VrfInput vrf_input(BytesIn domain, BytesIn data);

  VrfInput vrf_input_from_data(BytesIn domain, std::span<BytesIn> data);

  VrfOutput vrf_output(BandersnatchSecretKey secret, VrfInput input);

  template <size_t N>
    requires(N == 16 or N == 32)
  common::Blob<N> make_bytes(VrfInput input, VrfOutput output);

  VrfSignData vrf_sign_data(BytesIn label,
                            std::span<BytesIn> data,
                            std::span<VrfInput> inputs);

  RingVrfSignature ring_vrf_sign(BandersnatchSecretKey secret,
                                 VrfSignData sign_data,
                                 RingProver ring_prover);

  RingSignature ring_sign(BandersnatchSecretKey secret,
                          VrfSignData sign_data,
                          RingProver ring_prover);

}  // namespace kagome::crypto::bandersnatch::vrf
