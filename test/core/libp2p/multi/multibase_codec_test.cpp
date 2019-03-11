/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"

using namespace libp2p::multi;
using namespace kagome::common;

class MultibaseCodecTest : public ::testing::Test {
 public:
  std::unique_ptr<MultibaseCodec> multibase =
      std::make_unique<MultibaseCodecImpl>();

  /**
   * Decode the string
   * @param encoded - string with encoding prefix to be decoded into bytes; MUST
   * be valid encoded string
   * @return resulting Multibase
   */
  Buffer decodeCorrect(std::string_view encoded) {
    auto multibase_opt = multibase->decode(encoded);
    EXPECT_TRUE(multibase_opt)
        << "failed to decode string: " + std::string{encoded};
    return multibase_opt.tryExtractValue();
  }
};

TEST_F(MultibaseCodecTest, EncodeEmptyBytes) {
  auto encoded_str =
      multibase->encode(Buffer{}, MultibaseCodec::Encoding::kBase16Lower);
  ASSERT_TRUE(encoded_str.empty());
}

/**
 * @given string with encoding prefix, which does not stand for any of the
 * implemented encodings
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(MultibaseCodecTest, DecodeIncorrectPrefix) {
  auto multibase_err = multibase->decode("J00AA");
  ASSERT_FALSE(multibase_err);
}

/**
 * @given string of length 1
 * @when trying to decode that string
 * @then Multibase object creation fails
 */
TEST_F(MultibaseCodecTest, DecodeFewCharacters) {
  auto multibase_err = multibase->decode("A");
  ASSERT_FALSE(multibase_err);
}

class Base16EncodingUpper : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::kBase16Upper;

  std::string_view encoded_correct{"F00010204081020FF"};
  Buffer decoded_correct{0, 1, 2, 4, 8, 16, 32, 255};

  std::string_view encoded_incorrect_prefix{"fAA"};
  std::string_view encoded_incorrect_body{"F10A"};
};

/**
 * @given uppercase hex-encoded string
 * @when trying to decode that string
 * @then decoding succeeds
 */
TEST_F(Base16EncodingUpper, SuccessDecoding) {
  auto decoded_bytes = decodeCorrect(encoded_correct);
  ASSERT_EQ(decoded_bytes, decoded_correct);
}

/**
 * @given bytes
 * @when trying to encode those bytes
 * @then encoding succeeds
 */
TEST_F(Base16EncodingUpper, SuccessEncoding) {
  auto encoded_str = multibase->encode(decoded_correct, encoding);
  ASSERT_EQ(encoded_str, encoded_correct);
}

/**
 * @given uppercase hex-encoded string with lowercase hex prefix
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base16EncodingUpper, IncorrectPrefix) {
  auto error = multibase->decode(encoded_incorrect_prefix);
  ASSERT_FALSE(error);
}

/**
 * @given non-hex-encoded string with uppercase prefix
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base16EncodingUpper, IncorrectBody) {
  auto error = multibase->decode(encoded_incorrect_body);
  ASSERT_FALSE(error);
}

class Base16EncodingLower : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::kBase16Lower;

  std::string_view encoded_correct{"f00010204081020ff"};
  Buffer decoded_correct{0, 1, 2, 4, 8, 16, 32, 255};

  std::string_view encoded_incorrect_prefix{"Faa"};
  std::string_view encoded_incorrect_body{"f10a"};
};

/**
 * @given lowercase hex-encoded string
 * @when trying to decode that string
 * @then decoding succeeds
 */
TEST_F(Base16EncodingLower, SuccessDecoding) {
  auto decoded_bytes = decodeCorrect(encoded_correct);
  ASSERT_EQ(decoded_bytes, decoded_correct);
}

/**
 * @given bytes
 * @when trying to encode those bytes
 * @then encoding succeeds
 */
TEST_F(Base16EncodingLower, SuccessEncoding) {
  auto encoded_str = multibase->encode(decoded_correct, encoding);
  ASSERT_EQ(encoded_str, encoded_correct);
}

/**
 * @given lowercase hex-encoded string with uppercase hex prefix
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base16EncodingLower, IncorrectPrefix) {
  auto error = multibase->decode(encoded_incorrect_prefix);
  ASSERT_FALSE(error);
}

/**
 * @given non-hex-encoded string with lowercase prefix
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base16EncodingLower, IncorrectBody) {
  auto error = multibase->decode(encoded_incorrect_body);
  ASSERT_FALSE(error);
}

class Base58Encoding : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::kBase58;

  const std::vector<std::pair<Buffer, std::string_view>> decode_encode_table{
      {Buffer{0x61}, "Z2g"},
      {Buffer{0x62, 0x62, 0x62}, "Za3gV"},
      {Buffer{0x63, 0x63, 0x63}, "ZaPEr"},
      {Buffer{0x73, 0x69, 0x6d, 0x70, 0x6c, 0x79, 0x20, 0x61, 0x20, 0x6c,
              0x6f, 0x6e, 0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67},
       "Z2cFupjhnEsSn59qHXstmK2ffpLv2"},
      {Buffer{0x00, 0xeb, 0x15, 0x23, 0x1d, 0xfc, 0xeb, 0x60, 0x92,
              0x58, 0x86, 0xb6, 0x7d, 0x06, 0x52, 0x99, 0x92, 0x59,
              0x15, 0xae, 0xb1, 0x72, 0xc0, 0x66, 0x47},
       "Z1NS17iag9jJgTHD1VXjvLCEnZuQ3rJDE9L"},
      {Buffer{0x51, 0x6b, 0x6f, 0xcd, 0x0f}, "ZABnLTmg"},
      {Buffer{0xbf, 0x4f, 0x89, 0x00, 0x1e, 0x67, 0x02, 0x74, 0xdd},
       "Z3SEo3LWLoPntC"},
      {Buffer{0x57, 0x2e, 0x47, 0x94}, "Z3EFU7m"},
      {Buffer{0xec, 0xac, 0x89, 0xca, 0xd9, 0x39, 0x23, 0xc0, 0x23, 0x21},
       "ZEJDM8drfXA6uyA"},
      {Buffer{0x10, 0xc8, 0x51, 0x1e}, "ZRt5zm"},
      {Buffer{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
       "Z1111111111"},
      {Buffer{0x00, 0x01, 0x11, 0xd3, 0x8e, 0x5f, 0xc9, 0x07, 0x1f, 0xfc, 0xd2,
              0x0b, 0x4a, 0x76, 0x3c, 0xc9, 0xae, 0x4f, 0x25, 0x2b, 0xb4, 0xe4,
              0x8f, 0xd6, 0x6a, 0x83, 0x5e, 0x25, 0x2a, 0xda, 0x93, 0xff, 0x48,
              0x0d, 0x6d, 0xd4, 0x3d, 0xc6, 0x2a, 0x64, 0x11, 0x55, 0xa5},
       "Z123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"},
      {Buffer{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
              0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
              0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
              0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
              0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
              0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41,
              0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c,
              0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
              0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62,
              0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d,
              0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
              0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
              0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
              0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
              0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4,
              0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
              0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
              0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5,
              0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
              0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb,
              0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6,
              0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1,
              0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc,
              0xfd, 0xfe, 0xff},
       "Z1cWB5HCBdLjAuqGGReWE3R3CguuwSjw6RHn39s2yuDRTS5NsBgNiFpWgAnEx6VQi8c"
       "sexkgYw3mdYrMHr8x9i7aEwP8kZ7vccXWqKDvGv3u1GxFKPuAkn8JCPPGDMf3vMMnbz"
       "m6Nh9zh1gcNsMvH3ZNLmP5fSG6DGbbi2tuwMWPthr4boWwCxf7ewSgNQeacyozhKDDQ"
       "Q1qL5fQFUW52QKUZDZ5fw3KXNQJMcNTcaB723LchjeKun7MuGW5qyCBZYzA1KjofN1g"
       "YBV3NqyhQJ3Ns746GNuf9N2pQPmHz4xpnSrrfCvy6TVVz5d4PdrjeshsWQwpZsZGzvb"
       "dAdN8MKV5QsBDY"}};

  static constexpr std::string_view incorrect_encoded{"Z1c0I5H"};
};

/**
 * @given table with base58-encoded strings with their bytes representations
 * @when encoding bytes @and decoding strings
 * @then encoding/decoding succeed @and relevant bytes and strings are
 * equivalent
 */
TEST_F(Base58Encoding, SuccessEncodingDecoding) {
  for (const auto &[decoded, encoded] : decode_encode_table) {
    auto encoded_str = multibase->encode(decoded, encoding);
    ASSERT_EQ(encoded_str, encoded);

    auto decoded_bytes = decodeCorrect(encoded);
    ASSERT_EQ(decoded_bytes, decoded);
  }
}

/**
 * @given string containing symbols, forbidden in base58
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base58Encoding, IncorrectBody) {
  auto error = multibase->decode(incorrect_encoded);
  ASSERT_FALSE(error);
}

/**
 * Check that whitespace characters are skipped as intended
 * @given base58-encoded string with several whitespaces @and valid base58
 * symbols in the middle @and more whitespaces
 * @when trying to decode that string
 * @then decoding succeeds
 */
TEST_F(Base58Encoding, SkipsWhitespacesSuccess) {
  constexpr auto base64_with_whitespaces = "Z \t\n\v\f\r 2g \r\f\v\n\t ";
  auto decoded_bytes = decodeCorrect(base64_with_whitespaces);

  ASSERT_EQ(decoded_bytes, Buffer{0x61});
}

/**
 * Check that unexpected symbol in the end prevents success decoding
 * @given base58-encoded string with several whitespaces @and valid base58
 * symbols in the middle @and more whitespaces @and base58 character
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base58Encoding, SkipsWhitespacesFailure) {
  constexpr auto base64_with_whitespaces = "Z \t\n\v\f\r skip \r\f\v\n\t a";
  auto error = multibase->decode(base64_with_whitespaces);

  ASSERT_FALSE(error);
}

class Base64Encoding : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::kBase64;

  const std::vector<std::pair<Buffer, std::string_view>> decode_encode_table{
      {Buffer{0x66}, "mZg=="},
      {Buffer{0x66, 0x6f}, "mZm8="},
      {Buffer{0x66, 0x6f, 0x6f}, "mZm9v"},
      {Buffer{0x66, 0x6f, 0x6f, 0x62}, "mZm9vYg=="},
      {Buffer{0x66, 0x6f, 0x6f, 0x62, 0x61}, "mZm9vYmE="},
      {Buffer{0x66, 0x6f, 0x6f, 0x62, 0x61, 0x72}, "mZm9vYmFy"},
      {Buffer{0x4d, 0x61, 0x6e, 0x20, 0x69, 0x73, 0x20, 0x64, 0x69, 0x73, 0x74,
              0x69, 0x6e, 0x67, 0x75, 0x69, 0x73, 0x68, 0x65, 0x64, 0x2c, 0x20,
              0x6e, 0x6f, 0x74, 0x20, 0x6f, 0x6e, 0x6c, 0x79, 0x20, 0x62, 0x79,
              0x20, 0x68, 0x69, 0x73, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
              0x2c, 0x20, 0x62, 0x75, 0x74, 0x20, 0x62, 0x79, 0x20, 0x74, 0x68,
              0x69, 0x73, 0x20, 0x73, 0x69, 0x6e, 0x67, 0x75, 0x6c, 0x61, 0x72,
              0x20, 0x70, 0x61, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x66, 0x72,
              0x6f, 0x6d, 0x20, 0x6f, 0x74, 0x68, 0x65, 0x72, 0x20, 0x61, 0x6e,
              0x69, 0x6d, 0x61, 0x6c, 0x73, 0x2c, 0x20, 0x77, 0x68, 0x69, 0x63,
              0x68, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x6c, 0x75, 0x73, 0x74,
              0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x6d, 0x69, 0x6e,
              0x64, 0x2c, 0x20, 0x74, 0x68, 0x61, 0x74, 0x20, 0x62, 0x79, 0x20,
              0x61, 0x20, 0x70, 0x65, 0x72, 0x73, 0x65, 0x76, 0x65, 0x72, 0x61,
              0x6e, 0x63, 0x65, 0x20, 0x6f, 0x66, 0x20, 0x64, 0x65, 0x6c, 0x69,
              0x67, 0x68, 0x74, 0x20, 0x69, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20,
              0x63, 0x6f, 0x6e, 0x74, 0x69, 0x6e, 0x75, 0x65, 0x64, 0x20, 0x61,
              0x6e, 0x64, 0x20, 0x69, 0x6e, 0x64, 0x65, 0x66, 0x61, 0x74, 0x69,
              0x67, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x67, 0x65, 0x6e, 0x65, 0x72,
              0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x6f, 0x66, 0x20, 0x6b, 0x6e,
              0x6f, 0x77, 0x6c, 0x65, 0x64, 0x67, 0x65, 0x2c, 0x20, 0x65, 0x78,
              0x63, 0x65, 0x65, 0x64, 0x73, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73,
              0x68, 0x6f, 0x72, 0x74, 0x20, 0x76, 0x65, 0x68, 0x65, 0x6d, 0x65,
              0x6e, 0x63, 0x65, 0x20, 0x6f, 0x66, 0x20, 0x61, 0x6e, 0x79, 0x20,
              0x63, 0x61, 0x72, 0x6e, 0x61, 0x6c, 0x20, 0x70, 0x6c, 0x65, 0x61,
              0x73, 0x75, 0x72, 0x65, 0x2e},
       "mTWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieS"
       "B0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhI"
       "Gx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBp"
       "biB0aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2x"
       "lZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3"
       "VyZS4="},
  };

  static constexpr std::string_view incorrect_encoded{"m1c0=5H"};
};

/**
 * @given table with base64-encoded strings with their bytes representations
 * @when encoding bytes @and decoding strings
 * @then encoding/decoding succeed @and relevant bytes and strings are
 * equivalent
 */
TEST_F(Base64Encoding, SuccessEncodingDecoding) {
  for (const auto &[decoded, encoded] : decode_encode_table) {
    auto encoded_str = multibase->encode(decoded, encoding);
    ASSERT_EQ(encoded_str, encoded);

    auto decoded_bytes = decodeCorrect(encoded);
    ASSERT_EQ(decoded_bytes, decoded);
  }
}

/**
 * @given string containing symbols, forbidden in base64
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base64Encoding, IncorrectBody) {
  auto error = multibase->decode(incorrect_encoded);
  ASSERT_FALSE(error);
}
