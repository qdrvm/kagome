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

Hash256 createHash(std::initializer_list<uint8_t> bytes) {
  Hash256 h;
  h.fill(0);
  std::copy(bytes.begin(), bytes.end(), h.begin());
  return h;
}

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
  BlockHeader block_header_{createHash({0}),  // parent_hash
                            2,                // number: number
                            createHash({1}),  // state_root
                            createHash({2}),  // extrinsic root
                            {5}};             // buffer: digest;
  ByteArray encoded_header_ = []() {
    Buffer h;
    // SCALE-encoded
    h.put(std::vector<uint8_t>(32, 0));  // parent_hash: hash256 with value 0
    h.putUint8(2).put(std::vector<uint8_t>(7, 0));  // number: 2
    h.putUint8(1).put(
        std::vector<uint8_t>(31, 0));  // state_root: hash256 with value 1
    h.putUint8(2).put(
        std::vector<uint8_t>(31, 0));  // extrinsic_root: hash256 with value 2
    h.putUint8(4).putUint8(5);         // digest: buffer with element 5
    return h.toVector();
  }();
  /// Extrinsic instance and corresponding scale representation
  Extrinsic extrinsic_{{1, 2, 3}};
  ByteArray encoded_extrinsic_{12, 1, 2, 3};
  /// block instance and corresponding scale representation
  Block block_{block_header_, {extrinsic_}};
  ByteArray encoded_block_ =
      Buffer(encoded_header_)
          .putBuffer({4,  // (1 << 2) number of extrinsics
                      12, 1, 2, 3})
          .toVector();  // extrinsic itself {1, 2, 3}
  /// Version instance and corresponding scale representation
  Version version_{
      "qwe",                                                // spec name
      "asd",                                                // impl_name
      1,                                                    // auth version
      2,                                                    // impl version
      {{array{'1', '2', '3', '4', '5', '6', '7', '8'}, 1},  // ApiId_1
       {array{'8', '7', '6', '5', '4', '3', '2', '1'}, 2}}  // ApiId_2
  };
  ByteArray encoded_version_{
      12,  'q', 'w', 'e',                      // spec name
      12,  'a', 's', 'd',                      // impl name
      1,   0,   0,   0,                        // auth version
      2,   0,   0,   0,                        // impl version
      8,                                       // collection of 2 items
      '1', '2', '3', '4', '5', '6', '7', '8',  // id1
      1,   0,   0,   0,                        // id1 version
      '8', '7', '6', '5', '4', '3', '2', '1',  // id2
      2,   0,   0,   0,                        // id2 version
  };
  /// block id variant number alternative and corresponding scale representation
  BlockId block_id_number_{1ull};
  ByteArray encoded_block_id_number_{1, 1, 0, 0, 0, 0, 0, 0, 0};
  /// block id variant hash alternative and corresponding scale representation
  BlockId block_id_hash_;
  ByteArray encoded_block_id_hash_ =
      "00"  // variant type order
      "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"_unhex;

  /// TransactionValidity variant instance as Valid alternative and
  /// corresponding scale representation
  Valid valid_transaction_{1,                    // priority
                           {{0, 1}, {2, 3}},     // requires
                           {{4, 5}, {6, 7, 8}},  // provides
                           2};                   // longivity
  ByteArray encoded_valid_transaction_{
      1,                       // variant type order
      1, 0, 0, 0, 0, 0, 0, 0,  // priority
      8,                       // compact-encoded collection size (2)
      // collection of collections
      8,     // compact-encoded collection size (2)
      0, 1,  // collection items
      8,     // compact-encoded collection size (2)
      2, 3,  // collection items
      8,     // compact-encoded collection size (2)
      // collection of collections
      8,                       // compact-encoded collection size (2)
      4, 5,                    // collection items
      12,                      // compact-encoded collection size (3)
      6, 7, 8,                 // collection items
      2, 0, 0, 0, 0, 0, 0, 0,  // longevity
  };
};

/**
 * @given predefined block header
 * @when encodeBlockHeader is applied
 * @then expected result obtained
 */
TEST_F(Primitives, EncodeBlockHeaderSuccess) {
  EXPECT_OUTCOME_TRUE(val, encode(block_header_));
  ASSERT_EQ(val, encoded_header_);
}

/**
 * @given scale-encoded block header
 * @when decodeBlockHeader is applied
 * @then decoded instance of BlockHeader matches predefined block header
 */
TEST_F(Primitives, DecodeBlockHeaderSuccess) {
  EXPECT_OUTCOME_TRUE(val, decode<BlockHeader>(encoded_header_))
  ASSERT_EQ(val.parent_hash, block_header_.parent_hash);
  ASSERT_EQ(val.number, block_header_.number);
  ASSERT_EQ(val.state_root, block_header_.state_root);
  ASSERT_EQ(val.extrinsics_root, block_header_.extrinsics_root);
  ASSERT_EQ(val.digest, block_header_.digest);
}

/**
 * @given predefined extrinsic containing sequence {12, 1, 2, 3}
 * @when encodeExtrinsic is applied
 * @then the same expected buffer obtained {12, 1, 2, 3}
 */
TEST_F(Primitives, EncodeExtrinsicSuccess) {
  EXPECT_OUTCOME_TRUE(val, encode(extrinsic_));
  ASSERT_EQ(val, encoded_extrinsic_);
}

/**
 * @given encoded extrinsic
 * @when decodeExtrinsic is applied
 * @then decoded instance of Extrinsic matches predefined block header
 */
TEST_F(Primitives, DecodeExtrinsicSuccess) {
  EXPECT_OUTCOME_TRUE(res, decode<Extrinsic>(encoded_extrinsic_))
  ASSERT_EQ(res.data, extrinsic_.data);
}

/**
 * @given predefined instance of Block
 * @when encodeBlock is applied
 * @then expected result obtained
 */
TEST_F(Primitives, EncodeBlockSuccess) {
  EXPECT_OUTCOME_TRUE(res, encode(block_));
  ASSERT_EQ(res, encoded_block_);
}

/**
 * @given encoded block
 * @when decodeBlock is applied
 * @then decoded instance of Block matches predefined Block instance
 */
TEST_F(Primitives, DecodeBlockSuccess) {
  EXPECT_OUTCOME_TRUE(val, decode<Block>(encoded_block_));
  auto &&h = val.header;
  ASSERT_EQ(h.parent_hash, block_header_.parent_hash);
  ASSERT_EQ(h.number, block_header_.number);
  ASSERT_EQ(h.state_root, block_header_.state_root);
  ASSERT_EQ(h.extrinsics_root, block_header_.extrinsics_root);
  ASSERT_EQ(h.digest, block_header_.digest);

  auto &&extrinsics = val.extrinsics;
  ASSERT_EQ(extrinsics.size(), 1);
  ASSERT_EQ(extrinsics[0].data, extrinsic_.data);
}

/// Version

/**
 * @given version instance and predefined match for encoded instance
 * @when codec_->encodeVersion() is applied
 * @then obtained result equal to predefined match
 */
TEST_F(Primitives, EncodeVersionSuccess) {
  EXPECT_OUTCOME_TRUE(val, encode(version_));
  ASSERT_EQ(val, encoded_version_);
}

/**
 * @given byte array containing encoded version and predefined version
 * @when codec_->decodeVersion() is applied
 * @then obtained result matches predefined version instance
 */
TEST_F(Primitives, DecodeVersionSuccess) {
  EXPECT_OUTCOME_TRUE(ver, decode<Version>(encoded_version_))
  ASSERT_EQ(ver.spec_name, version_.spec_name);
  ASSERT_EQ(ver.impl_name, version_.impl_name);
  ASSERT_EQ(ver.authoring_version, version_.authoring_version);
  ASSERT_EQ(ver.apis, version_.apis);
}

/// BlockId

/**
 * @given block id constructed as hash256 and predefined match
 * @when codec_->encodeBlockId() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeBlockIdHash256Success) {
  EXPECT_OUTCOME_TRUE(val, encode(block_id_hash_))
  ASSERT_EQ(val, encoded_block_id_hash_);
}

/**
 * @given block id constructed as BlockNumber and predefined match
 * @when codec_->encodeBlockId() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeBlockIdBlockNumberSuccess) {
  EXPECT_OUTCOME_TRUE(val, encode(block_id_number_))
  ASSERT_EQ(val, (encoded_block_id_number_));
}

/**
 * @given byte array containing encoded block id
 * and predefined block id as Hash256
 * @when codec_->decodeBlockId() is applied
 * @thenobtained result matches predefined block id instance
 */
TEST_F(Primitives, DecodeBlockIdHashSuccess) {
  EXPECT_OUTCOME_TRUE(val, decode<BlockId>(encoded_block_id_hash_));
  // ASSERT_EQ has problems with pretty-printing variants
  ASSERT_TRUE(val == block_id_hash_);
}

/**
 * @given byte array containing encoded block id
 * and predefined block id as BlockNumber
 * @when codec_->decodeBlockId() is applied
 * @thenobtained result matches predefined block id instance
 */
TEST_F(Primitives, DecodeBlockIdNumberSuccess) {
  EXPECT_OUTCOME_TRUE(val, decode<BlockId>(encoded_block_id_number_))
  // ASSERT_EQ has problems with pretty-printing variants
  ASSERT_TRUE(val == block_id_number_);
}

/// TransactionValidity

/**
 * @given TransactionValidity instance as Invalid and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeTransactionValidityInvalidSuccess) {
  TransactionValidity invalid = Invalid{1};
  EXPECT_OUTCOME_TRUE(val, encode(invalid))
  ASSERT_EQ(val, (ByteArray{0, 1}));
}

/**
 * @given TransactionValidity instance as Unknown and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeTransactionValidityUnknown) {
  TransactionValidity unknown = Unknown{2};
  EXPECT_OUTCOME_TRUE(val, encode(unknown))
  ASSERT_EQ(val, (ByteArray{2, 2}));
}

/**
 * @given TransactionValidity instance as Valid and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, EncodeTransactionValiditySuccess) {
  TransactionValidity t = valid_transaction_; // make it variant type
  EXPECT_OUTCOME_TRUE(val, encode(t))
  ASSERT_EQ(val, encoded_valid_transaction_);
}

/**
 * @given byte array containing encoded
 * and TransactionValidity instance as Valid
 * @when codec_->decodeBlockId() is applied
 * @thenobtained result matches predefined block id instance
 */
TEST_F(Primitives, DecodeTransactionValidityInvalidSuccess) {
  Buffer bytes = {0, 1};
  EXPECT_OUTCOME_TRUE(val, decode<TransactionValidity>(bytes))
  kagome::visit_in_place(
      val,                                               // value
      [](Invalid const &v) { ASSERT_EQ(v.error_, 1); },  // ok
      [](Unknown const &v) { FAIL(); },                  // fail
      [](Valid const &v) { FAIL(); });                   // fail
}

/**
 * @given byte array containing encoded
 * and TransactionValidity instance as Valid
 * @when codec_->decodeBlockId() is applied
 * @then obtained result matches predefined block id instance
 */
TEST_F(Primitives, DecodeTransactionValidityUnknownSuccess) {
  Buffer bytes = {2, 2};
  auto &&res = decode<TransactionValidity>(bytes);
  ASSERT_TRUE(res);
  TransactionValidity value = res.value();
  kagome::visit_in_place(
      value,                                             // value
      [](Invalid const &v) { FAIL(); },                  // fail
      [](Unknown const &v) { ASSERT_EQ(v.error_, 2); },  // ok
      [](Valid const &v) { FAIL(); }                     // fail
  );
}

/**
 * @given byte array containing encoded
 * and TransactionValidity instance as Valid
 * @when codec_->decodeBlockId() is applied
 * @then obtained result matches predefined block id instance
 */
TEST_F(Primitives, DecodeTransactionValidityValidSuccess) {
  EXPECT_OUTCOME_TRUE(val, decode<TransactionValidity>(encoded_valid_transaction_))
  kagome::visit_in_place(
      val,                               // value
      [](Invalid const &v) { FAIL(); },  // fail
      [](Unknown const &v) { FAIL(); },  // fail
      [this](Valid const &v) {           // ok
        auto &valid = valid_transaction_;
        ASSERT_EQ(v.priority_, valid.priority_);
        ASSERT_EQ(v.requires_, valid.requires_);
        ASSERT_EQ(v.provides_, valid.provides_);
        ASSERT_EQ(v.longevity_, valid.longevity_);
      });
}

/**
 * @given vector of authority ids
 * @when encode and decode this vector using scale codec
 * @then decoded vector of authority ids matches the original one
 */
TEST_F(Primitives, EncodeDecodeAuthorityIdsSuccess) {
  AuthorityId id1, id2;
  id1.fill(1u);
  id2.fill(2u);
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
