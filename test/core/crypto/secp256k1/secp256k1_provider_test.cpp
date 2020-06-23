/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/secp256k1_provider_impl.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::crypto;
using namespace secp256k1;

/**
 * @brief Pre-generated key pair and signature for sample message
 * @using reference implementation from github.com/libp2p/go-libp2p-core
 */

class Secp256k1ProviderTest : public ::testing::Test {
 protected:
  /**
   * Sample message, signature, and pubkey from go-secp256k1
   * (https://github.com/ipsn/go-secp256k1/blob/master/secp256_test.go#L206)
   * Pay attention that hash function is not used for message (it is already
   * 32 byte long)
   */
  std::vector<uint8_t> secp_public_key_bytes{
      "04e32df42865e97135acfb65f3bae71bdc86f4d49150ad6a440b6f15878109880a0a2b2667f7e725ceea70c673093bf67663e0312623c8e091b13cf2c0f11ef652"_unhex};
  std::vector<uint8_t> secp_public_key_compressed_bytes{
      "02e32df42865e97135acfb65f3bae71bdc86f4d49150ad6a440b6f15878109880a"_unhex};
  std::vector<uint8_t> secp_signature_bytes{
      "90f27b8b488db00b00606796d2987f6a5f59ae62ea05effe84fef5b8b0e549984a691139ad57a3f0b906637673aa2f63d1f55cb1a69199d4009eea23ceaddc9301"_unhex};
  std::vector<uint8_t> secp_message_vector{
      "ce0677bb30baa8cf067c88db9811f4333d131bf8bcf12fe7065d211dce971008"_unhex};
  MessageHash secp_message_hash{};

  RSVSignature secp_signature{};
  ExpandedPublicKey secp_public_key{};
  CompressedPublicKey secp_public_key_compressed{};

  std::shared_ptr<Secp256k1Provider> secp256K1_provider =
      std::make_shared<Secp256k1ProviderImpl>();

  void SetUp() override {
    std::copy_n(secp_message_vector.begin(),
                secp_message_vector.size(),
                secp_message_hash.begin());
    std::copy_n(secp_public_key_bytes.begin(),
                secp_public_key_bytes.size(),
                secp_public_key.begin());
    std::copy_n(secp_public_key_compressed_bytes.begin(),
                secp_public_key_compressed_bytes.size(),
                secp_public_key_compressed.begin());
    std::copy_n(secp_signature_bytes.begin(),
                secp_signature_bytes.size(),
                secp_signature.begin());
  }
};

/**
 * @given Sample message and wrong signature
 * @when Recover pubkey from message and signature
 * @then Recovery must be unsuccessful
 */
TEST_F(Secp256k1ProviderTest, RecoverInvalidSignatureFailure) {
  RSVSignature wrong_signature{};
  *wrong_signature.rbegin() = 99;
  EXPECT_OUTCOME_ERROR(res,
                       secp256K1_provider->recoverPublickeyUncompressed(
                           wrong_signature, secp_message_hash),
                       Secp256k1ProviderError::INVALID_ARGUMENT);
}

/**
 * @given Sample message, signature, and pubkey from go-secp256k1
 * (https://github.com/ipsn/go-secp256k1/blob/master/secp256_test.go#L206)
 * Pay attention that hash function is not used for message (it is already 32
 * byte long)
 * @when Recover uncompressed pubkey from message and signature
 * @then Recovery is successful, public key returned
 */
TEST_F(Secp256k1ProviderTest, RecoverUncompressedSuccess) {
  EXPECT_OUTCOME_TRUE(public_key,
                      secp256K1_provider->recoverPublickeyUncompressed(
                          secp_signature, secp_message_hash));
  EXPECT_EQ(public_key, secp_public_key);
}

/**
 * @given Sample message, signature, and pubkey from go-secp256k1
 * (https://github.com/ipsn/go-secp256k1/blob/master/secp256_test.go#L206)
 * Pay attention that hash function is not used for message (it is already 32
 * byte long)
 * @when Recover compressed pubkey from message and signature
 * @then Recovery is successful, public key returned
 */
TEST_F(Secp256k1ProviderTest, RecoverCompressedSuccess) {
  EXPECT_OUTCOME_TRUE(public_key,
                      secp256K1_provider->recoverPublickeyCompressed(
                          secp_signature, secp_message_hash));
  EXPECT_EQ(public_key, secp_public_key_compressed);
}
