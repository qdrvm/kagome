/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase.hpp"
#include <gtest/gtest.h>
#include "utils/result_fixture.hpp"

using namespace libp2p::multi;
using namespace kagome::expected::testing;
using namespace kagome::common;

class MultibaseTest : public ::testing::Test {
 public:
  Multibase createCorrectFromEncoded(std::string_view encoded) {
    return *val(Multibase::createMultibaseFromEncoded(encoded))->value;
  }

  Multibase createCorrectFromDecoded(const Buffer &decoded,
                                     Multibase::Encoding encoding) {
    return *val(Multibase::createMultibaseFromDecoded(decoded, encoding))
                ->value;
  }
};

class Base16EncodingUpper : public MultibaseTest {
 public:
  Multibase::Encoding encoding = Multibase::Encoding::kBase16Upper;

  std::string_view encoded_correct{"F00010204081020FF"};
  Buffer decoded_correct{0, 1, 2, 4, 8, 16, 32, 255};

  std::string_view encoded_incorrect_no_prefix{"100"};
  std::string_view encoded_incorrect_prefix{"fAA"};
  std::string_view encoded_incorrect_body{"F10A"};
};

TEST_F(Base16EncodingUpper, CreateFromEncodedSuccess) {
  auto multibase = createCorrectFromEncoded(encoded_correct);
  ASSERT_EQ(multibase.base(), encoding);
  ASSERT_EQ(multibase.decodedData(), decoded_correct);
  ASSERT_EQ(multibase.encodedData(), encoded_correct);
}

TEST_F(Base16EncodingUpper, CreateFromDecodedSuccess) {
  auto multibase = createCorrectFromDecoded(decoded_correct, encoding);
  ASSERT_EQ(multibase.base(), encoding);
  ASSERT_EQ(multibase.decodedData(), decoded_correct);
  ASSERT_EQ(multibase.encodedData(), encoded_correct);
}

TEST_F(Base16EncodingUpper, CreateFromEncodedNoPrefix) {
  auto multibase_err =
      err(Multibase::createMultibaseFromEncoded(encoded_incorrect_no_prefix));
  ASSERT_TRUE(multibase_err);
}

TEST_F(Base16EncodingUpper, CreateFromEncodedIncorrectPrefix) {
  auto multibase_err =
      err(Multibase::createMultibaseFromEncoded(encoded_incorrect_prefix));
  ASSERT_TRUE(multibase_err);
}

TEST_F(Base16EncodingUpper, CreateFromEncodedIncorrectBody) {
  auto multibase_err =
      err(Multibase::createMultibaseFromEncoded(encoded_incorrect_body));
  ASSERT_TRUE(multibase_err);
}

TEST_F(Base16EncodingUpper, CreateFromEncodedFewCharacters) {
  auto multibase_err =
      err(Multibase::createMultibaseFromEncoded("A"));
  ASSERT_TRUE(multibase_err);
}

class Base16EncodingLower : public MultibaseTest {};

TEST_F(Base16EncodingLower, CreateFromEncodedSuccess) {}

TEST_F(Base16EncodingLower, CreateFromDecodedSuccess) {}
