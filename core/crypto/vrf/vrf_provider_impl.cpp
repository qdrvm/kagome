/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrf_provider_impl.hpp"

#include <gsl/span>
#include "crypto/util.hpp"

namespace kagome::crypto {

  VRFProviderImpl::VRFProviderImpl(std::shared_ptr<CSPRNG> generator)
      : generator_{std::move(generator)} {}

  SR25519Keypair VRFProviderImpl::generateKeypair() const {
    auto seed = generator_->randomBytes(SR25519_SEED_SIZE);

    std::vector<uint8_t> kp(SR25519_KEYPAIR_SIZE, 0);
    sr25519_keypair_from_seed(kp.data(), seed.data());

    return SR25519Keypair{kp};
  }

  boost::optional<VRFOutput> VRFProviderImpl::sign(
      const common::Buffer &msg,
      const SR25519Keypair &keypair,
      const VRFValue &threshold) const {
    common::Buffer keypair_buf{};
    keypair_buf.put(keypair.secret_key).put(keypair.public_key);

    std::array<uint8_t, SR25519_VRF_OUTPUT_SIZE + SR25519_VRF_PROOF_SIZE>
        out_proof{};

    auto sign_res =
        sr25519_vrf_sign_if_less(out_proof.data(),
                                 keypair_buf.data(),
                                 msg.data(),
                                 msg.size(),
                                 util::uint256_t_to_bytes(threshold).data());
    if (not sign_res.is_less) {
      return boost::none;
    }

    VRFOutput res;
    auto out_proof_span = gsl::make_span(out_proof);
    res.value = util::bytes_to_uint256_t(
        out_proof_span.subspan(0, SR25519_VRF_OUTPUT_SIZE));
    std::copy(out_proof.begin() + SR25519_VRF_OUTPUT_SIZE,
              out_proof.end(),
              res.proof.begin());

    return res;
  }

  bool VRFProviderImpl::verify(const common::Buffer &msg,
                               const VRFOutput &output,
                               const SR25519PublicKey &public_key) const {
    std::array<uint8_t, SR25519_VRF_OUTPUT_SIZE> out_array =
        util::uint256_t_to_bytes(output.value);

    auto res = sr25519_vrf_verify(public_key.data(),
                                  msg.data(),
                                  msg.size(),
                                  out_array.data(),
                                  output.proof.data());
    return res == Sr25519SignatureResult::Ok;
  }

}  // namespace kagome::crypto
