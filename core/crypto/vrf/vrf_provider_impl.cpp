/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
      const VRFThreshold &threshold) const {
    common::Buffer keypair_buf{};
    keypair_buf.put(keypair.secret_key).put(keypair.public_key);

    std::array<uint8_t, vrf_constants::OUTPUT_SIZE + vrf_constants::PROOF_SIZE>
        out_proof{};
    auto threshold_bytes = common::uint128_t_to_bytes(threshold);
    auto sign_res =
        sr25519_vrf_sign_if_less(out_proof.data(),
                                 keypair_buf.data(),
                                 msg.data(),
                                 msg.size(),
                                 threshold_bytes.data());
    if (not sign_res.is_less or not (sign_res.result == Sr25519SignatureResult::Ok)) {
      return boost::none;
    }

    VRFOutput res;
    std::copy_n(
        out_proof.begin(), vrf_constants::OUTPUT_SIZE, res.output.begin());
    std::copy_n(out_proof.begin() + vrf_constants::OUTPUT_SIZE,
                vrf_constants::PROOF_SIZE,
                res.proof.begin());

    return res;
  }

  VRFVerifyOutput VRFProviderImpl::verify(const common::Buffer &msg,
                                          const VRFOutput &output,
                                          const SR25519PublicKey &public_key,
                                          const VRFThreshold &threshold) const {
    auto res = sr25519_vrf_verify(public_key.data(),
                                  msg.data(),
                                  msg.size(),
                                  output.output.data(),
                                  output.proof.data(),
                                  common::uint128_t_to_bytes(threshold).data());
    return VRFVerifyOutput{.is_valid = res.result == Sr25519SignatureResult::Ok,
                           .is_less = res.is_less};
  }

}  // namespace kagome::crypto
