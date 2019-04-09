/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/impl/crypto_provider_impl.hpp"

using kagome::common::Buffer;
using namespace libp2p::crypto;

class AesFixture : public testing::Test {
 public:
  using Aes128Secret = libp2p::crypto::common::Aes128Secret;
  using Aes256Secret = libp2p::crypto::common::Aes256Secret;

  AesFixture() {
    CryptoProviderImpl::initializeOpenSSL();
  }

  aes::AesCrypt provider;
};

TEST_F(AesFixture, simple) {
  auto key = Buffer::fromHex("0x06a9214036b8a15b512e03d534120006").getValue();
  auto iv = Buffer::fromHex("0x3dafba429d9eb430b422da802c9fac41").getValue();

  Buffer plain_text;
  plain_text.put("Single block msg");

  auto cipher_text_result = Buffer::fromHex("0xe353779c1079aeb82708942dbe77181a").getValue();

//  auto secret = Aes128Secret();
//  provider.encrypt128()

}
