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

#include <gtest/gtest.h>
#include "crypto/random_generator/boost_generator.hpp"
#include "common/mp_utils.hpp"

using kagome::common::Buffer;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::SR25519Keypair;
using kagome::crypto::VRFProviderImpl;
using kagome::crypto::VRFPreOutput;
using kagome::crypto::VRFThreshold;

class VRFProviderTest : public testing::Test {
 public:
  void SetUp() override {
    vrf_provider_ = std::make_shared<VRFProviderImpl>(
        std::make_shared<BoostRandomGenerator>());
    keypair1_ = vrf_provider_->generateKeypair();
    keypair2_ = vrf_provider_->generateKeypair();
    msg_ = Buffer{1, 2, 3};
  }

 protected:
  std::shared_ptr<VRFProviderImpl> vrf_provider_;
  SR25519Keypair keypair1_;
  SR25519Keypair keypair2_;
  Buffer msg_;
};

/**
 * @given vrf provider @and very large threshold value @and some message
 * @when we derive vrf value and proof from signing the message
 * @then output value is less than threshold @and proof verifies that value was
 * generated using vrf
 */
TEST_F(VRFProviderTest, SignAndVerifySuccess) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::max() - 1};
  // when
  auto out_opt = vrf_provider_->sign(msg_, keypair1_, threshold);
  ASSERT_TRUE(out_opt);
  auto out = out_opt.value();

  // then
  auto verify_res = vrf_provider_->verify(msg_, out, keypair1_.public_key, threshold);
  ASSERT_TRUE(verify_res.is_valid);
  ASSERT_TRUE(verify_res.is_less);
}

/**
 * @given vrf provider @and very large threshold value @and some message
 * @when we derive vrf value and proof from signing the message @and try to
 * verify proof by wrong public key
 * @then output value is less than threshold @and proof is not verified
 */
TEST_F(VRFProviderTest, VerifyFailed) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::max() - 1};

  // when
  auto out_opt = vrf_provider_->sign(msg_, keypair1_, threshold);
  ASSERT_TRUE(out_opt);
  auto out = out_opt.value();

  // then
  ASSERT_FALSE(vrf_provider_->verify(msg_, out, keypair2_.public_key, threshold).is_valid);
}

/**
 * @given vrf provider @and very small threshold value @and some message
 * @when we try to derive vrf output from signing the message
 * @then output is not created as value is bigger than threshold
 */
TEST_F(VRFProviderTest, SignFailed) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFPreOutput>::min()};

  // when
  auto out_opt = vrf_provider_->sign(msg_, keypair1_, threshold);

  // then
  ASSERT_FALSE(out_opt.has_value());
}
