/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_PROVIDER_MOCK_HPP
#define KAGOME_CRYPTO_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/crypto/crypto_provider.hpp"

namespace libp2p::crypto {
  class CryptoProviderMock : public CryptoProvider {
    MOCK_CONST_METHOD2(aesEncrypt,
                       kagome::common::Buffer(const common::Aes128Secret &,
                                              const kagome::common::Buffer &));

    MOCK_CONST_METHOD2(aesEncrypt,
                       kagome::common::Buffer(const common::Aes256Secret &,
                                              const kagome::common::Buffer &));

    MOCK_CONST_METHOD2(aesDecrypt,
                       kagome::common::Buffer(const common::Aes128Secret &,
                                              const kagome::common::Buffer &));

    MOCK_CONST_METHOD2(aesDecrypt,
                       kagome::common::Buffer(const common::Aes256Secret &,
                                              const kagome::common::Buffer &));

    MOCK_CONST_METHOD3(hmacDigest,
                       kagome::common::Buffer(common::HashType,
                                              const kagome::common::Buffer &,
                                              const kagome::common::Buffer &));

    MOCK_CONST_METHOD0(generateEd25519Keypair, common::KeyPair());

    MOCK_CONST_METHOD1(generateRSAKeypair, common::KeyPair(common::RSAKeyType));

    MOCK_CONST_METHOD1(generateEphemeralKeyPair,
                       common::EphemeralKeyPair(common::CurveType));

    MOCK_CONST_METHOD3(
        keyStretcher,
        std::vector<common::StretchedKey>(common::CipherType, common::HashType,
                                          const kagome::common::Buffer &));

    MOCK_CONST_METHOD1(marshal, kagome::common::Buffer(const PublicKey &));

    MOCK_CONST_METHOD1(marshal, kagome::common::Buffer(const PrivateKey &));

    MOCK_CONST_METHOD1(
        unmarshalPublicKey,
        std::unique_ptr<PublicKey>(const kagome::common::Buffer &));

    MOCK_CONST_METHOD1(
        unmarshalPrivateKey,
        std::unique_ptr<PrivateKey>(const kagome::common::Buffer &));

    MOCK_CONST_METHOD2(import,
                       std::unique_ptr<PrivateKey>(boost::filesystem::path,
                                                   std::string_view));

    MOCK_CONST_METHOD1(randomBytes, kagome::common::Buffer(size_t));

    MOCK_CONST_METHOD5(pbkdf2,
                       kagome::common::Buffer(std::string_view,
                                              const kagome::common::Buffer &,
                                              uint64_t, size_t,
                                              common::HashType));
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_CRYPTO_PROVIDER_MOCK_HPP
