/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/variant.hpp>
#include <outcome/outcome.hpp>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/visitor.hpp"
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
#include "testutil/primitives/hash_creator.hpp"

using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::ApiId;
using kagome::primitives::AuthorityId;
using kagome::primitives::Block;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::primitives::InherentIdentifier;
using kagome::primitives::Invalid;
using kagome::primitives::TransactionValidity;
using kagome::primitives::Unknown;
using kagome::primitives::Valid;
using kagome::primitives::Version;
using kagome::scale::ByteArray;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

using kagome::scale::decode;
using kagome::scale::encode;

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
  BlockHeader block_header_{createHash256({0}),  // parent_hash
                            2,                   // number: number
                            createHash256({1}),  // state_root
                            createHash256({2}),  // extrinsic root
                            {{5}}};              // buffer: digest;
  /// Extrinsic instance and corresponding scale representation
  Extrinsic extrinsic_{{1, 2, 3}};
  /// block instance and corresponding scale representation
  Block block_{block_header_, {extrinsic_}};
  /// Version instance and corresponding scale representation
  Version version_{
      "qwe",  // spec name
      "asd",  // impl_name
      1,      // auth version
      2,      // impl version
      {{Blob{array{'1', '2', '3', '4', '5', '6', '7', '8'}}, 1},  // ApiId_1
       {Blob{array{'8', '7', '6', '5', '4', '3', '2', '1'}}, 2}}  // ApiId_2
  };
  /// block id variant number alternative and corresponding scale representation
  BlockId block_id_number_{1ull};
  /// block id variant hash alternative and corresponding scale representation
  BlockId block_id_hash_;

  /// TransactionValidity variant instance as Valid alternative and
  /// corresponding scale representation
  Valid valid_transaction_{1,                    // priority
                           {{0, 1}, {2, 3}},     // requires
                           {{4, 5}, {6, 7, 8}},  // provides
                           2};                   // longivity
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
  Invalid invalid{1};
  EXPECT_OUTCOME_TRUE(val, encode(invalid))
  EXPECT_OUTCOME_TRUE(decoded_validity, decode<Invalid>(val));
  ASSERT_EQ(decoded_validity, invalid);
}

/**
 * @given TransactionValidity instance as Unknown and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeTransactionValidityUnknown) {
  Unknown unknown{2};
  EXPECT_OUTCOME_TRUE(val, encode(unknown))
  EXPECT_OUTCOME_TRUE(decoded_validity, decode<Unknown>(val));
  ASSERT_EQ(decoded_validity, unknown);
}

/**
 * @given TransactionValidity instance as Valid and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeTransactionValiditySuccess) {
  Valid t = valid_transaction_;  // make it variant type
  EXPECT_OUTCOME_TRUE(val, encode(t))
  EXPECT_OUTCOME_TRUE(decoded_validity, decode<Valid>(val));
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

/**
 * @given inherent data
 * @when  encode and decode it
 * @then decoded result is exactly the original inherent data
 */
TEST_F(Primitives, EncodeInherentDataSuccess) {
  using array = std::array<uint8_t, 8u>;
  InherentData data;
  InherentIdentifier id1{array{1}};
  InherentIdentifier id2{array{2}};
  InherentIdentifier id3{array{3}};
  Buffer b1{1, 2, 3, 4};
  Buffer b2{5, 6, 7, 8};
  Buffer b3{1, 2, 3, 4};
  EXPECT_OUTCOME_TRUE_void(r1, data.putData(id1, b1));
  EXPECT_OUTCOME_TRUE_void(r2, data.putData(id2, b2));
  EXPECT_OUTCOME_TRUE_void(r3, data.putData(id3, b3));
  EXPECT_OUTCOME_FALSE_void(_, data.putData(id1, b2));

  ASSERT_EQ(data.getData(id1).value(), b1);
  ASSERT_EQ(data.getData(id2).value(), b2);
  ASSERT_EQ(data.getData(id3).value(), b3);

  Buffer b4{1, 3, 5, 7};
  data.replaceData(id3, b4);
  ASSERT_EQ(data.getData(id3).value(), b4);

  EXPECT_OUTCOME_TRUE(enc_data, encode(data))
  EXPECT_OUTCOME_TRUE(dec_data, decode<InherentData>(enc_data))

  EXPECT_EQ(data.getDataCollection(), dec_data.getDataCollection());
}
