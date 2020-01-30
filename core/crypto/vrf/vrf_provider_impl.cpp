/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrf_provider_impl.hpp"

#include <gsl/span>

#include "common/mp_utils.hpp"

namespace kagome::crypto {
  namespace vrf_constants = constants::sr25519::vrf;

  VRFProviderImpl::VRFProviderImpl(std::shared_ptr<CSPRNG> generator)
      : generator_{std::move(generator)} {}

  SR25519Keypair VRFProviderImpl::generateKeypair() const {
    auto seed = generator_->randomBytes(constants::sr25519::SEED_SIZE);

    std::array<uint8_t, constants::sr25519::KEYPAIR_SIZE> kp{};
    sr25519_keypair_from_seed(kp.data(), seed.data());

    SR25519Keypair keypair;
    std::copy(kp.begin(),
              kp.begin() + constants::sr25519::SECRET_SIZE,
              keypair.secret_key.begin());
    std::copy(kp.begin() + constants::sr25519::SECRET_SIZE,
              kp.begin() + constants::sr25519::SECRET_SIZE
                  + constants::sr25519::PUBLIC_SIZE,
              keypair.public_key.begin());
    return keypair;
  }

  boost::optional<VRFOutput> VRFProviderImpl::sign(
      const common::Buffer &msg,
      const SR25519Keypair &keypair,
      const VRFRawOutput &threshold) const {
    common::Buffer keypair_buf{};
    keypair_buf.put(keypair.secret_key).put(keypair.public_key);

    std::array<uint8_t, vrf_constants::OUTPUT_SIZE + vrf_constants::PROOF_SIZE>
        out_proof{};
    std::array<uint8_t, vrf_constants::RAW_OUTPUT_SIZE> out_raw{};

    auto sign_res =
        sr25519_vrf_sign_if_less(out_proof.data(),
                                 keypair_buf.data(),
                                 msg.data(),
                                 msg.size(),
                                 common::uint256_t_to_bytes(threshold).data());
    if (not sign_res.is_less) {
      return boost::none;
    }

    VRFOutput res;
    auto out_proof_span = gsl::make_span(out_proof);
    res.output = common::bytes_to_uint256_t(
        out_proof_span.subspan(0, vrf_constants::OUTPUT_SIZE));
    res.raw_output = common::bytes_to_uint128_t(out_raw);
    std::copy(out_proof.begin() + vrf_constants::OUTPUT_SIZE,
              out_proof.end(),
              res.proof.begin());

    return res;
  }

  VRFVerifyOutput VRFProviderImpl::verify(const common::Buffer &msg,
                               const VRFOutput &output,
                               const SR25519PublicKey &public_key,
                               const VRFRawOutput &threshold) const {
    std::array<uint8_t, vrf_constants::OUTPUT_SIZE> out_array =
        common::uint256_t_to_bytes(output.output);

    auto res = sr25519_vrf_verify(public_key.data(),
                                  msg.data(),
                                  msg.size(),
                                  out_array.data(),
                                  output.proof.data(),
                                  common::uint256_t_to_bytes(threshold).data()
                                  );
    return VRFVerifyOutput {.is_valid = res.result == Sr25519SignatureResult::Ok, .is_less = res.is_less};
  }

}  // namespace kagome::crypto
