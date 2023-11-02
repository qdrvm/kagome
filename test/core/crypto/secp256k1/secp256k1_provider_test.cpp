/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/secp256k1_provider_impl.hpp"

#include "crypto/hasher/hasher_impl.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::crypto::Hasher;
using kagome::crypto::HasherImpl;
using kagome::crypto::Secp256k1Provider;
using kagome::crypto::Secp256k1ProviderError;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::secp256k1::CompressedPublicKey;
using kagome::crypto::secp256k1::MessageHash;
using kagome::crypto::secp256k1::RSVSignature;
using kagome::crypto::secp256k1::UncompressedPublicKey;

/**
 * @brief Pre-generated key pair and signature for sample message
 * to generate test data following script was used:
 * https://gist.github.com/masterjedy/c6fe4a2c654c10b30da000153318eeb1
 */
struct Secp256k1ProviderTest : public ::testing::Test {
  MessageHash secp_message_hash;
  RSVSignature secp_signature;
  UncompressedPublicKey secp_public_key_expanded;
  CompressedPublicKey secp_public_key_compressed;

  std::shared_ptr<Secp256k1Provider> secp256K1_provider =
      std::make_shared<Secp256k1ProviderImpl>();
  std::shared_ptr<Hasher> hasher = std::make_shared<HasherImpl>();

  void SetUp() override {
    // message: "this is a message"
    EXPECT_OUTCOME_TRUE(secp_message_vector,
                        Buffer::fromHex("746869732069732061206d657373616765"));
    secp_message_hash = hasher->blake2s_256(secp_message_vector);

    EXPECT_OUTCOME_TRUE(
        secp_public_key_expanded_bytes,
        Buffer::fromHex("04f821bc128a43d9b0516969111e19a40bab417f45181d692d0519"
                        "a3b35573cb63178403d12eb41d7702913a70ebc1c64438002a1474"
                        "e1328276b7dcdacb511fc3"));
    secp_public_key_expanded =
        UncompressedPublicKey::fromSpan(secp_public_key_expanded_bytes).value();

    EXPECT_OUTCOME_TRUE(secp_public_key_compressed_bytes,
                        Buffer::fromHex("03f821bc128a43d9b0516969111e19a40bab41"
                                        "7f45181d692d0519a3b35573cb63"));
    secp_public_key_compressed =
        CompressedPublicKey::fromSpan(secp_public_key_compressed_bytes).value();

    EXPECT_OUTCOME_TRUE(
        secp_signature_bytes,
        Buffer::fromHex("ebdedee38bcf530f13c1b5c8717d974a6f8bd25a7e3707ca36c7ee"
                        "7efd5aa6c557bcc67906975696cbb28a556b649e5fbf5ce5183157"
                        "2cd54add248c4d023fcf01"));
    secp_signature = RSVSignature::fromSpan(secp_signature_bytes).value();
  }
};

/**
 * @given Sample message and invalid RSV-signature formed from valid one
 * by corrupting recovery id
 * @when Recover pubkey from message and signature
 * @then Recovery fails with `invalid v value` error
 */
TEST_F(Secp256k1ProviderTest, RecoverInvalidRecidFailure) {
  RSVSignature wrong_signature = secp_signature;
  *wrong_signature.rbegin() = 0xFF;
  EXPECT_OUTCOME_ERROR(res,
                       secp256K1_provider->recoverPublickeyUncompressed(
                           wrong_signature, secp_message_hash),
                       Secp256k1ProviderError::INVALID_V_VALUE);
}

/**
 * @given Sample message and invalid RSV-signature formed from valid one
 * by corrupting signature itself
 * @when Recover pubkey from message and signature
 * @then Recovery fails with `invalid signature` error
 */
TEST_F(Secp256k1ProviderTest, RecoverInvalidSignatureFailure) {
  RSVSignature wrong_signature = secp_signature;
  *(wrong_signature.begin() + 3) = 0xFF;
  auto res = secp256K1_provider->recoverPublickeyUncompressed(
      wrong_signature, secp_message_hash);
  // corrupted signature may recover some public key, but this key is wrong
  if (res) {
    ASSERT_NE(res.value(), secp_public_key_expanded);
  } else {
    // otherwise the operation should result in failure
    EXPECT_OUTCOME_ERROR(err, res, Secp256k1ProviderError::INVALID_SIGNATURE);
  }
}

/**
 * @given Sample message, signature, and pubkey
 * @when Recover uncompressed pubkey from message and signature
 * @then Recovery is successful, public key returned
 */
TEST_F(Secp256k1ProviderTest, RecoverUncompressedSuccess) {
  EXPECT_OUTCOME_TRUE(public_key,
                      secp256K1_provider->recoverPublickeyUncompressed(
                          secp_signature, secp_message_hash));
  EXPECT_EQ(public_key, secp_public_key_expanded);
}

/**
 * @given Sample message, signature, and pubkey
 * @when Recover compressed pubkey from message and signature
 * @then Recovery is successful, public key is returned
 */
TEST_F(Secp256k1ProviderTest, RecoverCompressedSuccess) {
  EXPECT_OUTCOME_TRUE(public_key,
                      secp256K1_provider->recoverPublickeyCompressed(
                          secp_signature, secp_message_hash));
  EXPECT_EQ(public_key, secp_public_key_compressed);
}
