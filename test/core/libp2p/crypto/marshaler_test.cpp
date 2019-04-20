/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/marshaler/key_marshaler.hpp"

#include <gtest/gtest.h>
#include "libp2p/crypto/common.hpp"

using kagome::common::Buffer;
using libp2p::crypto::PrivateKey;
using libp2p::crypto::PublicKey;
using libp2p::crypto::common::KeyType;
using libp2p::crypto::marshaler::KeyMarshaler;

class MarshalerFixture : public testing::Test {
 public:
  KeyMarshaler marshaler_;
};

/**
 * script used for generating test cases:
 * https://gist.github.com/8f0bae63e8e20b40ec481b9ae8903879
 */

/**
 *  @given public key of type UNSPECIFIED
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPublicKey_UNSPECIFIED) {
  PublicKey key = {KeyType::UNSPECIFIED,
                   {110, 206, 16, 137, 211, 132, 92, 156, 203, 247, 121, 180,
                    104, 102, 253, 244}};
  Buffer match = {18,  16,  110, 206, 16,  137, 211, 132, 92,
                  156, 203, 247, 121, 180, 104, 102, 253, 244};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type UNSPECIFIED
 *  @when unmarshalPublicKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPublicKey_UNSPECIFIED) {
  Buffer bytes = {18,  16,  110, 206, 16,  137, 211, 132, 92,
                  156, 203, 247, 121, 180, 104, 102, 253, 244};
  PublicKey key_match = {KeyType::UNSPECIFIED,
                         {110, 206, 16, 137, 211, 132, 92, 156, 203, 247, 121,
                          180, 104, 102, 253, 244}};
  auto &&res = marshaler_.unmarshalPublicKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given private key of type UNSPECIFIED
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPrivateKey_UNSPECIFIED) {
  PrivateKey key = {KeyType::UNSPECIFIED,
                    {110, 206, 16, 137, 211, 132, 92, 156, 203, 247, 121, 180,
                     104, 102, 253, 244}};
  Buffer match = {18,  16,  110, 206, 16,  137, 211, 132, 92,
                  156, 203, 247, 121, 180, 104, 102, 253, 244};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type UNSPECIFIED
 *  @when unmarshalPrivateKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPrivateKey_UNSPECIFIED) {
  Buffer bytes = {18,  16,  110, 206, 16,  137, 211, 132, 92,
                  156, 203, 247, 121, 180, 104, 102, 253, 244};
  PublicKey key_match = {KeyType::UNSPECIFIED,
                         {110, 206, 16, 137, 211, 132, 92, 156, 203, 247, 121,
                          180, 104, 102, 253, 244}};
  auto &&res = marshaler_.unmarshalPrivateKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given public key of type RSA1024
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPublicKey_RSA1024) {
  PublicKey key = {KeyType::RSA1024,
                   {169, 94, 64, 111, 149, 96, 150, 216, 18, 24, 27, 124, 202,
                    219, 152, 158}};
  Buffer match = {8,   1,   18, 16, 169, 94,  64,  111, 149, 96,
                  150, 216, 18, 24, 27,  124, 202, 219, 152, 158};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type RSA1024
 *  @when unmarshalPublicKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPublicKey_RSA1024) {
  Buffer bytes = {8,   1,   18, 16, 169, 94,  64,  111, 149, 96,
                  150, 216, 18, 24, 27,  124, 202, 219, 152, 158};
  PublicKey key_match = {KeyType::RSA1024,
                         {169, 94, 64, 111, 149, 96, 150, 216, 18, 24, 27, 124,
                          202, 219, 152, 158}};
  auto &&res = marshaler_.unmarshalPublicKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given private key of type RSA1024
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPrivateKey_RSA1024) {
  PrivateKey key = {KeyType::RSA1024,
                    {169, 94, 64, 111, 149, 96, 150, 216, 18, 24, 27, 124, 202,
                     219, 152, 158}};
  Buffer match = {8,   1,   18, 16, 169, 94,  64,  111, 149, 96,
                  150, 216, 18, 24, 27,  124, 202, 219, 152, 158};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type RSA1024
 *  @when unmarshalPrivateKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPrivateKey_RSA1024) {
  Buffer bytes = {8,   1,   18, 16, 169, 94,  64,  111, 149, 96,
                  150, 216, 18, 24, 27,  124, 202, 219, 152, 158};
  PublicKey key_match = {KeyType::RSA1024,
                         {169, 94, 64, 111, 149, 96, 150, 216, 18, 24, 27, 124,
                          202, 219, 152, 158}};
  auto &&res = marshaler_.unmarshalPrivateKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given public key of type RSA2048
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPublicKey_RSA2048) {
  PublicKey key = {KeyType::RSA2048,
                   {250, 113, 250, 232, 107, 191, 193, 38, 235, 136, 62, 238,
                    198, 238, 253, 137}};
  Buffer match = {8,   2,  18,  16,  250, 113, 250, 232, 107, 191,
                  193, 38, 235, 136, 62,  238, 198, 238, 253, 137};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type RSA2048
 *  @when unmarshalPublicKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPublicKey_RSA2048) {
  Buffer bytes = {8,   2,  18,  16,  250, 113, 250, 232, 107, 191,
                  193, 38, 235, 136, 62,  238, 198, 238, 253, 137};
  PublicKey key_match = {KeyType::RSA2048,
                         {250, 113, 250, 232, 107, 191, 193, 38, 235, 136, 62,
                          238, 198, 238, 253, 137}};
  auto &&res = marshaler_.unmarshalPublicKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given private key of type RSA2048
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPrivateKey_RSA2048) {
  PrivateKey key = {KeyType::RSA2048,
                    {250, 113, 250, 232, 107, 191, 193, 38, 235, 136, 62, 238,
                     198, 238, 253, 137}};
  Buffer match = {8,   2,  18,  16,  250, 113, 250, 232, 107, 191,
                  193, 38, 235, 136, 62,  238, 198, 238, 253, 137};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type RSA2048
 *  @when unmarshalPrivateKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPrivateKey_RSA2048) {
  Buffer bytes = {8,   2,  18,  16,  250, 113, 250, 232, 107, 191,
                  193, 38, 235, 136, 62,  238, 198, 238, 253, 137};
  PublicKey key_match = {KeyType::RSA2048,
                         {250, 113, 250, 232, 107, 191, 193, 38, 235, 136, 62,
                          238, 198, 238, 253, 137}};
  auto &&res = marshaler_.unmarshalPrivateKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given public key of type RSA4096
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPublicKey_RSA4096) {
  PublicKey key = {
      KeyType::RSA4096,
      {201, 183, 191, 144, 26, 148, 57, 168, 114, 71, 3, 58, 121, 163, 1, 228}};
  Buffer match = {8,  3,   18,  16, 201, 183, 191, 144, 26, 148,
                  57, 168, 114, 71, 3,   58,  121, 163, 1,  228};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type RSA4096
 *  @when unmarshalPublicKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPublicKey_RSA4096) {
  Buffer bytes = {8,  3,   18,  16, 201, 183, 191, 144, 26, 148,
                  57, 168, 114, 71, 3,   58,  121, 163, 1,  228};
  PublicKey key_match = {
      KeyType::RSA4096,
      {201, 183, 191, 144, 26, 148, 57, 168, 114, 71, 3, 58, 121, 163, 1, 228}};
  auto &&res = marshaler_.unmarshalPublicKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given private key of type RSA4096
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPrivateKey_RSA4096) {
  PrivateKey key = {
      KeyType::RSA4096,
      {201, 183, 191, 144, 26, 148, 57, 168, 114, 71, 3, 58, 121, 163, 1, 228}};
  Buffer match = {8,  3,   18,  16, 201, 183, 191, 144, 26, 148,
                  57, 168, 114, 71, 3,   58,  121, 163, 1,  228};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type RSA4096
 *  @when unmarshalPrivateKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPrivateKey_RSA4096) {
  Buffer bytes = {8,  3,   18,  16, 201, 183, 191, 144, 26, 148,
                  57, 168, 114, 71, 3,   58,  121, 163, 1,  228};
  PublicKey key_match = {
      KeyType::RSA4096,
      {201, 183, 191, 144, 26, 148, 57, 168, 114, 71, 3, 58, 121, 163, 1, 228}};
  auto &&res = marshaler_.unmarshalPrivateKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given public key of type ED25519
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPublicKey_ED25519) {
  PublicKey key = {KeyType::ED25519,
                   {166, 90, 244, 10, 68, 237, 35, 185, 74, 22, 117, 193, 129,
                    102, 100, 201}};
  Buffer match = {8,  4,   18, 16, 166, 90,  244, 10,  68,  237,
                  35, 185, 74, 22, 117, 193, 129, 102, 100, 201};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type ED25519
 *  @when unmarshalPublicKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPublicKey_ED25519) {
  Buffer bytes = {8,  4,   18, 16, 166, 90,  244, 10,  68,  237,
                  35, 185, 74, 22, 117, 193, 129, 102, 100, 201};
  PublicKey key_match = {KeyType::ED25519,
                         {166, 90, 244, 10, 68, 237, 35, 185, 74, 22, 117, 193,
                          129, 102, 100, 201}};
  auto &&res = marshaler_.unmarshalPublicKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}

/**
 *  @given private key of type ED25519
 *  @when marshal is applied
 *  @then obtained result matches predefined sequence of bytes
 */
TEST_F(MarshalerFixture, marshalPrivateKey_ED25519) {
  PrivateKey key = {KeyType::ED25519,
                    {166, 90, 244, 10, 68, 237, 35, 185, 74, 22, 117, 193, 129,
                     102, 100, 201}};
  Buffer match = {8,  4,   18, 16, 166, 90,  244, 10,  68,  237,
                  35, 185, 74, 22, 117, 193, 129, 102, 100, 201};
  auto &&res = marshaler_.marshal(key);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val, match);
}

/**
 *  @given sequence of bytes and predefined public key of type ED25519
 *  @when unmarshalPrivateKey is applied
 *  @then obtained result matches predefined public key
 */
TEST_F(MarshalerFixture, unmarshalPrivateKey_ED25519) {
  Buffer bytes = {8,  4,   18, 16, 166, 90,  244, 10,  68,  237,
                  35, 185, 74, 22, 117, 193, 129, 102, 100, 201};
  PublicKey key_match = {KeyType::ED25519,
                         {166, 90, 244, 10, 68, 237, 35, 185, 74, 22, 117, 193,
                          129, 102, 100, 201}};
  auto &&res = marshaler_.unmarshalPrivateKey(bytes);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.getType(), key_match.getType());
  ASSERT_EQ(val.getBytes(), key_match.getBytes());
}
