/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/variant.hpp>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/visitor.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/inherent_data.hpp"
#include "primitives/parachain_host.hpp"
#include "primitives/scheduled_change.hpp"
#include "primitives/session_key.hpp"
#include "primitives/transaction_validity.hpp"
#include "primitives/version.hpp"
#include "scale/scale.hpp"
#include "scale/scale_error.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/mp_utils.hpp"

using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::ApiId;
using kagome::primitives::AuthorityId;
using kagome::primitives::Block;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::Digest;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::primitives::InherentIdentifier;
using kagome::primitives::InvalidTransaction;
using kagome::primitives::PreRuntime;
using kagome::primitives::TransactionValidity;
using kagome::primitives::UnknownTransaction;
using kagome::primitives::ValidTransaction;
using kagome::primitives::Version;
using scale::ByteArray;
using scale::ScaleDecoderStream;
using scale::ScaleEncoderStream;

using scale::decode;
using scale::encode;

using testutil::createHash256;

/**
 * @class Primitives is a test fixture which contains useful data
 */
class Primitives : public testing::Test {
  void SetUp() override {
    block_id_hash_ = Hash256::fromHex(
                         "000102030405060708090A0B0C0D0E0F"
                         "101112131415161718191A1B1C1D1E1F")
                         .value();
  }

 protected:
  using array = std::array<uint8_t, 8u>;
  /// block header and corresponding scale representation
  BlockHeader block_header_{2,                      // number: number
                            createHash256({0}),     // parent_hash
                            createHash256({1}),     // state_root
                            createHash256({2}),     // extrinsic root
                            Digest{PreRuntime{}}};  // digest;
  /// Extrinsic instance and corresponding scale representation
  Extrinsic extrinsic_{{1, 2, 3}};
  /// block instance and corresponding scale representation
  Block block_{block_header_, {extrinsic_}};
  /// Version instance and corresponding scale representation
  Version version_{
      "qwe",  // spec name
      "asd",  // impl_name
      1,      // auth version
      42,     // spec version
      2,      // db version
      {{Blob{array{'1', '2', '3', '4', '5', '6', '7', '8'}}, 1},   // ApiId_1
       {Blob{array{'8', '7', '6', '5', '4', '3', '2', '1'}}, 2}},  // ApiId_2
      1,  // transaction version
      0,  // state version
  };
  /// block id variant number alternative and corresponding scale representation
  BlockId block_id_number_{1ull};
  /// block id variant hash alternative and corresponding scale representation
  BlockId block_id_hash_;

  /// TransactionValidity variant instance as Valid alternative and
  /// corresponding scale representation
  ValidTransaction valid_transaction_{.priority = 1,
                                      .required_tags = {{0, 1}, {2, 3}},
                                      .provided_tags = {{4, 5}, {6, 7, 8}},
                                      .longevity = 2,
                                      .propagate = true};
};

/**
 * @given predefined block header
 * @when encodeBlockHeader is applied and result is decoded back
 * @then decoded block is equal to predefined block
 */
TEST_F(Primitives, EncodeBlockHeaderSuccess) {
  EXPECT_OUTCOME_TRUE(val, encode(block_header_));
  EXPECT_OUTCOME_TRUE(decoded_header, decode<BlockHeader>(val));
  ASSERT_EQ(block_header_, decoded_header);
}

/**
 * @given predefined extrinsic containing sequence {12, 1, 2, 3}
 * @when encodeExtrinsic is applied
 * @then the same expected buffer obtained {12, 1, 2, 3}
 */
TEST_F(Primitives, EncodeExtrinsicSuccess) {
  EXPECT_OUTCOME_TRUE(val, encode(extrinsic_));
  EXPECT_OUTCOME_TRUE(decoded_extrinsic, decode<Extrinsic>(val));
  ASSERT_EQ(extrinsic_, decoded_extrinsic);
}

/**
 * @given predefined instance of Block
 * @when encodeBlock is applied
 * @then expected result obtained
 */
TEST_F(Primitives, EncodeBlockSuccess) {
  EXPECT_OUTCOME_TRUE(res, encode(block_));
  EXPECT_OUTCOME_TRUE(decoded_block, decode<Block>(res));
  ASSERT_EQ(block_, decoded_block);
}

/// Version

/**
 * @given version instance and predefined match for encoded instance
 * @when codec_->encodeVersion() is applied
 * @then obtained result equal to predefined match
 */
TEST_F(Primitives, EncodeVersionSuccess) {
  EXPECT_OUTCOME_TRUE(val, encode(version_));
  EXPECT_OUTCOME_TRUE(decoded_version, decode<Version>(val));
  ASSERT_EQ(decoded_version, version_);
}

/// BlockId

/**
 * @given block id constructed as hash256 and predefined match
 * @when codec_->encodeBlockId() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeBlockIdHash256Success) {
  EXPECT_OUTCOME_TRUE(val, encode(block_id_hash_))
  EXPECT_OUTCOME_TRUE(decoded_block_id, decode<BlockId>(val));
  ASSERT_EQ(decoded_block_id, block_id_hash_);
}

/**
 * @given block id constructed as BlockNumber and predefined match
 * @when codec_->encodeBlockId() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeBlockIdBlockNumberSuccess) {
  EXPECT_OUTCOME_TRUE(val, encode(block_id_number_))
  EXPECT_OUTCOME_TRUE(decoded_block_id, decode<BlockId>(val));
  ASSERT_EQ(decoded_block_id, block_id_number_);
}

/// TransactionValidity

/**
 * @given TransactionValidity instance as Invalid and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeTransactionValidityInvalidSuccess) {
  InvalidTransaction invalid{1};
  EXPECT_OUTCOME_TRUE(val, encode(invalid))
  EXPECT_OUTCOME_TRUE(decoded_validity, decode<InvalidTransaction>(val));
  ASSERT_EQ(decoded_validity, invalid);
}

/**
 * @given TransactionValidity instance as Unknown and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeTransactionValidityUnknown) {
  UnknownTransaction unknown{2};
  EXPECT_OUTCOME_TRUE(val, encode(unknown))
  EXPECT_OUTCOME_TRUE(decoded_validity, decode<UnknownTransaction>(val));
  ASSERT_EQ(decoded_validity, unknown);
}

/**
 * @given TransactionValidity instance as Valid and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeTransactionValiditySuccess) {
  ValidTransaction t = valid_transaction_;  // make it variant type
  EXPECT_OUTCOME_TRUE(val, encode(t))
  EXPECT_OUTCOME_TRUE(decoded_validity, decode<ValidTransaction>(val));
  ASSERT_EQ(decoded_validity, valid_transaction_);
}

/**
 * @given vector of authority ids
 * @when encode and decode this vector using scale codec
 * @then decoded vector of authority ids matches the original one
 */
TEST_F(Primitives, EncodeDecodeAuthorityIdsSuccess) {
  AuthorityId id1, id2;
  id1.id.fill(1u);
  id2.id.fill(2u);
  std::vector<AuthorityId> original{id1, id2};
  EXPECT_OUTCOME_TRUE(res, encode(original))

  EXPECT_OUTCOME_TRUE(decoded, decode<std::vector<AuthorityId>>(res))
  ASSERT_EQ(original, decoded);
}

TEST_F(Primitives, EncodeInherentSampleFromSubstrate) {
  std::string encoded_str = {'\b', 't',  'e',  's',    't',  'i',  'n',  'h',
                             '0',  '4',  '\f', '\x01', '\0', '\0', '\0', '\x02',
                             '\0', '\0', '\0', '\x03', '\0', '\0', '\0', 't',
                             'e',  's',  't',  'i',    'n',  'h',  '1',  '\x10',
                             '\a', '\0', '\0', '\0'};

  EXPECT_OUTCOME_TRUE(test_id1, InherentIdentifier::fromString("testinh0"));
  std::vector<uint32_t> data1{1, 2, 3};

  EXPECT_OUTCOME_TRUE(test_id2, InherentIdentifier::fromString("testinh1"));
  uint32_t data2 = 7;

  auto encoded_buf = Buffer().put(encoded_str);
  EXPECT_OUTCOME_TRUE(dec_data, decode<InherentData>(encoded_buf));

  EXPECT_OUTCOME_TRUE(dec_data1, dec_data.getData<decltype(data1)>(test_id1));
  EXPECT_EQ(data1, dec_data1);

  EXPECT_OUTCOME_TRUE(dec_data2, dec_data.getData<decltype(data2)>(test_id2));
  EXPECT_EQ(data2, dec_data2);
}

/**
 * @given inherent data
 * @when  encode and decode it
 * @then decoded result is exactly the original inherent data
 */
TEST_F(Primitives, EncodeInherentDataSuccess) {
  using array = std::array<uint8_t, 8u>;
  InherentData data;
  EXPECT_OUTCOME_TRUE(id1, InherentIdentifier::fromString("testinh0"));
  EXPECT_OUTCOME_TRUE(id2, InherentIdentifier::fromString("testinh1"));
  InherentIdentifier id3{array{3}};
  std::vector<uint32_t> data1{1, 2, 3};
  uint32_t data2 = 7;
  Buffer data3{1, 2, 3, 4};
  EXPECT_OUTCOME_TRUE_void(r1, data.putData(id1, data1));
  EXPECT_OUTCOME_TRUE_void(r2, data.putData(id2, data2));
  EXPECT_OUTCOME_TRUE_void(r3, data.putData(id3, data3));
  EXPECT_OUTCOME_FALSE_void(_, data.putData(id1, data2));

  ASSERT_EQ(data.getData<decltype(data1)>(id1).value(), data1);
  ASSERT_EQ(data.getData<decltype(data2)>(id2).value(), data2);
  ASSERT_EQ(data.getData<decltype(data3)>(id3).value(), data3);

  Buffer data4{1, 3, 5, 7};
  data.replaceData(id3, data4);
  ASSERT_EQ(data.getData<decltype(data4)>(id3).value(), data4);

  EXPECT_OUTCOME_TRUE(enc_data, encode(data))
  EXPECT_OUTCOME_TRUE(dec_data, decode<InherentData>(enc_data))

  ASSERT_EQ(dec_data.getData<decltype(data1)>(id1).value(), data1);

  EXPECT_EQ(data, dec_data);
}
