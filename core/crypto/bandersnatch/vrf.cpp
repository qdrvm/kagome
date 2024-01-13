/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch/vrf.hpp"

namespace kagome::crypto::bandersnatch::vrf {

  VrfInput vrf_input(BytesIn domain, BytesIn data) {
    auto vrf_input = ::bandersnatch_vrf_input(
        domain.data(), domain.size(), data.data(), data.size());
    return VrfInput(vrf_input);
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

  VrfOutput vrf_output(const BandersnatchSecretKey &secret,
                       const VrfInput &input) {
    auto output_ptr = ::bandersnatch_vrf_output(secret.data(), input.ptr);
    VrfOutput output;
    ::bandersnatch_vrf_output_encode(output_ptr, output.data());
    return output;
  }

  template <>
  common::Blob<16> make_bytes<16>(BytesIn context,
                                  const VrfInput &input,
                                  const VrfOutput &output) {
    auto output_ptr = ::bandersnatch_vrf_output_decode(output.data());
    common::Blob<16> bytes;
    ::bandersnatch_make_bytes(context.data(),
                              context.size(),
                              input.ptr,
                              output_ptr,
                              bytes.data(),
                              bytes.size());
    return bytes;
  }

  template <>
  common::Blob<32> make_bytes<32>(BytesIn context,
                                  const VrfInput &input,
                                  const VrfOutput &output) {
    auto output_ptr = ::bandersnatch_vrf_output_decode(output.data());
    common::Blob<32> bytes;
    ::bandersnatch_make_bytes(context.data(),
                              context.size(),
                              input.ptr,
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

    BOOST_ASSERT(inputs.size() <= MAX_VRF_IOS);
    std::vector<const ::bandersnatch_VrfInput *> input_ptrs;
    for (auto &input : inputs) {
      input_ptrs.push_back(input.ptr);
    }

    auto res = ::bandersnatch_vrf_sign_data(label.data(),
                                            label.size(),
                                            data_ptrs.data(),
                                            data_sizes.data(),
                                            data.size(),
                                            input_ptrs.data(),
                                            input_ptrs.size());
    return VrfSignData(res);
  }

  template <>
  common::Blob<32> vrf_sign_data_challenge(const VrfSignData &sign_data) {
    common::Blob<32> challenge;
    ::bandersnatch_sign_data_challenge(
        sign_data.ptr, challenge.data(), challenge.size());
    return challenge;
  }

  VrfSignature vrf_sign(const BandersnatchSecretKey &secret_key,
                        const VrfSignData &sign_data) {
    auto signature_ptr =
        ::bandersnatch_vrf_sign(secret_key.data(), sign_data.ptr);

    static const size_t maxEncodedSize = 1
                                       + MAX_VRF_IOS * BANDERSNATCH_PREOUT_SIZE
                                       + BANDERSNATCH_SIGNATURE_SIZE;
    common::Blob<maxEncodedSize> buff;
    ::bandersnatch_vrf_signature_encode(signature_ptr, buff.data());

    auto res = scale::decode<VrfSignature>(buff).value();
    return res;
  }

  bool vrf_verify(const VrfSignature &signature,
                  const VrfSignData &sign_data,
                  const BandersnatchPublicKey &public_key) {
    auto buff = scale::encode(signature).value();

    auto signature_ptr =
        ::bandersnatch_vrf_signature_decode(buff.data(), buff.size());

    if (signature_ptr == nullptr) {
      return false;
    }

    auto res = ::bandersnatch_vrf_verify(
        signature_ptr, sign_data.ptr, public_key.data());

    return res;
  }

  RingVrfSignature ring_vrf_sign(const BandersnatchSecretKey &secret_key,
                                 const VrfSignData &sign_data,
                                 const RingProver &ring_prover) {
    auto ring_signature_ptr = ::bandersnatch_ring_vrf_sign(
        secret_key.data(), sign_data.ptr, ring_prover.ptr);

    static const size_t maxEncodedSize = 1
                                       + MAX_VRF_IOS * BANDERSNATCH_PREOUT_SIZE
                                       + BANDERSNATCH_RING_SIGNATURE_SIZE;
    common::Blob<maxEncodedSize> buff;
    ::bandersnatch_ring_vrf_signature_encode(ring_signature_ptr, buff.data());

    auto res = scale::decode<RingVrfSignature>(buff).value();
    return res;
  }

  bool ring_vrf_verify(const RingVrfSignature &signature,
                       const VrfSignData &sign_data,
                       const RingVerifier &ring_verifier) {
    auto buff = scale::encode(signature).value();

    auto signature_ptr =
        ::bandersnatch_ring_vrf_signature_decode(buff.data(), buff.size());

    auto res = ::bandersnatch_ring_vrf_verify(
        signature_ptr, sign_data.ptr, ring_verifier.ptr);
    return res;
  }

}  // namespace kagome::crypto::bandersnatch::vrf
