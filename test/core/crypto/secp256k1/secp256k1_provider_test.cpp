/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/secp256k1_provider_impl.hpp"

#include "crypto/hasher/hasher_impl.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::crypto;
using namespace secp256k1;

using kagome::common::Blob;
using kagome::common::Buffer;

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
  Buffer secp_public_key_expanded_bytes{
      "04f821bc128a43d9b0516969111e19a40bab417f45181d692d0519a3b35573cb63178403d12eb41d7702913a70ebc1c64438002a1474e1328276b7dcdacb511fc3"_hex2buf};
  Buffer secp_public_key_compressed_bytes{
      "03f821bc128a43d9b0516969111e19a40bab417f45181d692d0519a3b35573cb63"_hex2buf};
  Buffer secp_signature_bytes{
      "c5a65afd7eeec736ca07377e5ad28b6f4a977d71c8b5c1130f53cf8be3dedeebcf3f024d8c24dd4ad52c573118e55cbf5f9e646b558ab2cb9656970679c6bc5701"_hex2buf};

  // message: "this is a message"
  Buffer secp_message_vector{"746869732069732061206d657373616765"_hex2buf};
  MessageHash secp_message_hash{};

  RSVSignature secp_signature{};
  ExpandedPublicKey secp_public_key_expanded{};
  CompressedPublicKey secp_public_key_compressed{};

  std::shared_ptr<Secp256k1Provider> secp256K1_provider =
      std::make_shared<Secp256k1ProviderImpl>();
  std::shared_ptr<kagome::crypto::Hasher> hasher =
      std::make_shared<kagome::crypto::HasherImpl>();

  void SetUp() override {
    auto msg_hash = hasher->blake2s_256(secp_message_vector);

    std::copy_n(msg_hash.begin(), Blob<32>::size(), secp_message_hash.begin());
    std::cout << "secp message hash = " << secp_message_hash.toHex()
              << std::endl;
    std::cout << "msg hash = " << msg_hash.toHex() << std::endl;
    std::copy_n(secp_public_key_expanded_bytes.begin(),
                secp_public_key_expanded_bytes.size(),
                secp_public_key_expanded.begin());
    std::copy_n(secp_public_key_compressed_bytes.begin(),
                secp_public_key_compressed_bytes.size(),
                secp_public_key_compressed.begin());
    std::copy_n(secp_signature_bytes.begin(),
                secp_signature_bytes.size(),
                secp_signature.begin());
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
  EXPECT_OUTCOME_ERROR(res,
                       secp256K1_provider->recoverPublickeyUncompressed(
                           wrong_signature, secp_message_hash),
                       Secp256k1ProviderError::INVALID_SIGNATURE);
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
  EXPECT_EQ(public_key, secp_public_key_expanded);
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
