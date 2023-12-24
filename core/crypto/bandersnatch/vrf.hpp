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

namespace kagome::crypto::bandersnatch::vrf {

  using libp2p::Bytes;
  using libp2p::BytesIn;
  using libp2p::BytesOut;

  using VrfInput = ::bandersnatch_VrfInput *;
  using VrfOutput = ::bandersnatch_VrfOutput *;
  using VrfSignData = ::bandersnatch_VrfSignData *;

  VrfInput vrf_input(BytesIn domain, BytesIn data);

  VrfInput vrf_input_from_data(BytesIn domain, std::span<BytesIn> data);

  VrfOutput vrf_output(BandersnatchSecretKey secret, VrfInput input);

  template <size_t N>
    requires(N == 16 or N == 32)
  common::Blob<N> make_bytes(VrfInput input, VrfOutput output);

  VrfSignData vrf_sign_data(libp2p::BytesIn label,
                            std::span<libp2p::BytesIn> data,
                            std::span<VrfInput> inputs);

}  // namespace kagome::crypto::bandersnatch::vrf
