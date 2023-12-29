/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch/vrf.hpp"

namespace kagome::crypto::bandersnatch::vrf {

  VrfInput vrf_input(BytesIn domain, BytesIn data) {
    return ::bandersnatch_vrf_input(
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
    auto output_ptr = ::bandersnatch_vrf_output(secret.data(), input);
    VrfOutput output;
    ::bandersnatch_vrf_output_encode(output_ptr, output.data());
    return output;
  }

  template <>
  common::Blob<16> make_bytes<16>(BytesIn context,
                                  VrfInput input,
                                  VrfOutput output) {
    auto output_ptr = ::bandersnatch_vrf_output_decode(output.data());
    common::Blob<16> bytes;
    ::bandersnatch_make_bytes(context.data(),
                              context.size(),
                              input,
                              output_ptr,
                              bytes.data(),
                              bytes.size());
    return bytes;
  }

  template <>
  common::Blob<32> make_bytes<32>(BytesIn context,
                                  VrfInput input,
                                  VrfOutput output) {
    auto output_ptr = ::bandersnatch_vrf_output_decode(output.data());
    common::Blob<32> bytes;
    ::bandersnatch_make_bytes(context.data(),
                              context.size(),
                              input,
                              output_ptr,
                              bytes.data(),
                              bytes.size());
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

    return ::bandersnatch_vrf_sign_data(label.data(),
                                        label.size(),
                                        data_ptrs.data(),
                                        data_sizes.data(),
                                        data.size(),
                                        inputs.data(),
                                        inputs.size());
  }

  template <>
  common::Blob<32> vrf_sign_data_challenge(vrf::VrfSignData sign_data) {
    common::Blob<32> challenge;
    ::bandersnatch_sign_data_challenge(
        sign_data, challenge.data(), challenge.size());
    return challenge;
  }

  VrfSignature vrf_sign(BandersnatchSecretKey secret_key,
                        VrfSignData sign_data) {
    auto signature_ptr = ::bandersnatch_vrf_sign(secret_key.data(), sign_data);

    static const size_t maxEncodedSize = 1
                                       + MAX_VRF_IOS * BANDERSNATCH_PREOUT_SIZE
                                       + BANDERSNATCH_SIGNATURE_SIZE;
    common::Blob<maxEncodedSize> buff;
    ::bandersnatch_vrf_signature_encode(signature_ptr, buff.data());

    return scale::decode<VrfSignature>(buff).value();
  }

  bool vrf_verify(const VrfSignature &signature,
                  VrfSignData sign_data,
                  const BandersnatchPublicKey &public_key) {
    auto buff = scale::encode(signature).value();

    auto signature_ptr =
        ::bandersnatch_vrf_signature_decode(buff.data(), buff.size());

    if (signature_ptr == nullptr) {
      return false;
    }

    return ::bandersnatch_vrf_verify(
        signature_ptr, sign_data, public_key.data());
  }

  RingVrfSignature ring_vrf_sign(BandersnatchSecretKey secret_key,
                                 VrfSignData sign_data,
                                 RingProver ring_prover) {
    auto ring_signature_ptr =
        ::bandersnatch_ring_vrf_sign(secret_key.data(), sign_data, ring_prover);

    static const size_t maxEncodedSize = 1
                                       + MAX_VRF_IOS * BANDERSNATCH_PREOUT_SIZE
                                       + BANDERSNATCH_RING_SIGNATURE_SIZE;
    common::Blob<maxEncodedSize> buff;
    ::bandersnatch_ring_vrf_signature_encode(ring_signature_ptr, buff.data());

    return scale::decode<RingVrfSignature>(buff).value();
  }

  bool ring_vrf_verify(const RingVrfSignature &signature,
                       VrfSignData sign_data,
                       RingVerifier ring_verifier) {
    auto buff = scale::encode(signature).value();

    auto signature_ptr =
        ::bandersnatch_ring_vrf_signature_decode(buff.data(), buff.size());

    return ::bandersnatch_ring_vrf_verify(
        signature_ptr, sign_data, ring_verifier);
  }

}  // namespace kagome::crypto::bandersnatch::vrf
