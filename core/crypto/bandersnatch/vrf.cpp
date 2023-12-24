/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch/vrf.hpp"

namespace kagome::crypto::bandersnatch::vrf {

  VrfInput vrf_input(BytesIn domain, BytesIn data) {
    return bandersnatch_vrf_input(
        domain.data(), domain.size(), data.data(), data.size());
  }

  VrfInput vrf_input_from_data(BytesIn domain, std::span<BytesIn> data) {
    common::Buffer buff;
    for (auto &chunk : data) {
      buff.insert(buff.end(), chunk.begin(), chunk.end());
      BOOST_ASSERT(chunk.size() <= std::numeric_limits<uint8_t>::max());
      buff.putUint8(chunk.size());
    }

    return vrf_input(domain, std::span(buff));
  }

  VrfOutput vrf_output(BandersnatchSecretKey secret, VrfInput input) {
    return bandersnatch_vrf_output(secret.data(), input);
  }

  template <>
  common::Blob<16> make_bytes<16>(VrfInput input, VrfOutput output) {
    common::Blob<16> bytes;
    bandersnatch_make_bytes(input, output, bytes.data(), bytes.size());
    return bytes;
  }

  template <>
  common::Blob<32> make_bytes<32>(VrfInput input, VrfOutput output) {
    common::Blob<32> bytes;
    bandersnatch_make_bytes(input, output, bytes.data(), bytes.size());
    return bytes;
  }

  VrfSignData vrf_sign_data(libp2p::BytesIn label,
                            std::span<libp2p::BytesIn> data,
                            std::span<VrfInput> inputs) {
    std::vector<const uint8_t *> data_ptrs;
    std::vector<size_t> data_sizes;
    for (auto &d : data) {
      data_ptrs.push_back(d.data());
      data_sizes.push_back(d.size());
    }

    return bandersnatch_vrf_sign_data(label.data(),
                                      label.size(),
                                      data_ptrs.data(),
                                      data_sizes.data(),
                                      data.size(),
                                      inputs.data(),
                                      inputs.size());
  }

}  // namespace kagome::crypto::bandersnatch::vrf
