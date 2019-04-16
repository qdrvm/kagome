/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/impl/scale_codec_impl.hpp"

#include <gtest/gtest.h>
#include <boost/variant.hpp>
#include <outcome/outcome.hpp>
#include "common/visitor.hpp"
#include "primitives/block.hpp"
#include "primitives/version.hpp"
#include "scale/byte_array_stream.hpp"

using kagome::common::Buffer;
using kagome::primitives::Block;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::Extrinsic;
using kagome::primitives::Invalid;
using kagome::primitives::ScaleCodec;
using kagome::primitives::ScaleCodecImpl;
using kagome::primitives::TransactionValidity;
using kagome::primitives::Unknown;
using kagome::primitives::Valid;
using kagome::primitives::Version;
using kagome::scale::ByteArrayStream;

/**
 * @class Primitives is a test fixture which contains useful data
 */
class Primitives : public testing::Test {
  void SetUp() override {
    block_id_hash_ = kagome::common::Hash256::fromHex(
                         "000102030405060708090A0B0C0D0E0F"
                         "101112131415161718191A1B1C1D1E1F")
                         .getValue();

    codec_ = std::make_shared<ScaleCodecImpl>();
  }

 protected:
  /// block header and corresponding scale representation
  BlockHeader block_header_{{1},   // buffer: parent_hash
                            2,     // number: number
                            {3},   // buffer: state_root
                            {4},   // buffer: extrinsic root
                            {5}};  // buffer: digest;
  Buffer encoded_header_{
      4, 1,                    // {1}
      2, 0, 0, 0, 0, 0, 0, 0,  // 2 as uint64
      4, 3,                    // {3}
      4, 4,                    // {4}
      4, 5                     // {5}
  };
  /// Extrinsic instance and corresponding scale representation
  Extrinsic extrinsic_{{12,  /// sequence of 3 bytes: 1, 2, 3; 12 == (3 << 2)
                        1, 2, 3}};
  Buffer encoded_extrinsic_{12, 1, 2, 3};
  /// block instance and corresponding scale representation
  Block block_{block_header_, {extrinsic_}};
  Buffer encoded_block_{4,  1,                    // {1}
                        2,  0, 0, 0, 0, 0, 0, 0,  // 2 as uint64
                        4,  3,                    // {3}
                        4,  4,                    // {4}
                        4,  5,                    // {5}
                        4,             // (1 << 2) number of extrinsics
                        12, 1, 2, 3};  // extrinsic itself {1, 2, 3}
  /// Version instance and corresponding scale representation
  Version version_{
      "qwe",                                           // spec name
      "asd",                                           // impl_name
      1,                                               // auth version
      2,                                               // impl version
      {{{'1', '2', '3', '4', '5', '6', '7', '8'}, 1},  // ApiId_1
       {{'8', '7', '6', '5', '4', '3', '2', '1'}, 2}}  // ApiId_2
  };
  Buffer encoded_version_{
      12, 'q', 'w', 'e',                           // spec name
      12, 'a', 's', 'd',                           // impl name
      1,  0,   0,   0,                             // auth version
      2,  0,   0,   0,                             // impl version
      8,                                           // collection of 2 items
      32, '1', '2', '3', '4', '5', '6', '7', '8',  // id1
      1,  0,   0,   0,                             // id1 version
      32, '8', '7', '6', '5', '4', '3', '2', '1',  // id2
      2,  0,   0,   0,                             // id2 version
  };
  /// block id variant number alternative and corresponding scale representation
  BlockId block_id_number_{1ull};
  Buffer encoded_block_id_number_{1, 1, 0, 0, 0, 0, 0, 0, 0};
  /// block id variant hash alternative and corresponding scale representation
  BlockId block_id_hash_;
  Buffer encoded_block_id_hash_{
      0,    // variant type order
      128,  // collection size compact-encoded = 32 << 2 + 0x00
      0x0,  0x1,  0x2,  0x3,  0x4,  0x5,  0x6,  0x7,  0x8,  0x9,  0xA,
      0xB,  0xC,  0xD,  0xE,  0xF,  0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
  /// TransactionValidity variant instance as Valid alternative and
  /// corresponding scale representation
  Valid valid_transaction_{1,                    // priority
                           {{0, 1}, {2, 3}},     // requires
                           {{4, 5}, {6, 7, 8}},  // provides
                           2};                   // longivity
  Buffer encoded_valid_transaction_{
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
  /// ScaleCodec impl instance
  std::shared_ptr<ScaleCodec> codec_;
};

/**
 * @given predefined block header
 * @when encodeBlockHeader is applied
 * @then expected result obtained
 */
TEST_F(Primitives, encodeBlockHeader) {
  auto &&res = codec_->encodeBlockHeader(block_header_);

  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), encoded_header_);
}

/**
 * @given scale-encoded block header
 * @when decodeBlockHeader is applied
 * @then decoded instance of BlockHeader matches predefined block header
 */
TEST_F(Primitives, decodeBlockHeader) {
  ByteArrayStream stream{encoded_header_};

  auto &&res = codec_->decodeBlockHeader(stream);
  ASSERT_TRUE(res);

  auto &&val = res.value();

  ASSERT_EQ(val.parentHash(), block_header_.parentHash());
  ASSERT_EQ(val.number(), block_header_.number());
  ASSERT_EQ(val.stateRoot(), block_header_.stateRoot());
  ASSERT_EQ(val.extrinsicsRoot(), block_header_.extrinsicsRoot());
  ASSERT_EQ(val.digest(), block_header_.digest());
}

/**
 * @given predefined extrinsic containing sequence {12, 1, 2, 3}
 * @when encodeExtrinsic is applied
 * @then the same expected buffer obtained {12, 1, 2, 3}
 */
TEST_F(Primitives, encodeExtrinsic) {
  auto &&res = codec_->encodeExtrinsic(extrinsic_);
  ASSERT_TRUE(res);

  auto &&val = res.value();
  ASSERT_EQ(val, encoded_extrinsic_);
}

/**
 * @given encoded extrinsic
 * @when decodeExtrinsic is applied
 * @then decoded instance of Extrinsic matches predefined block header
 */
TEST_F(Primitives, decodeExtrinsic) {
  ByteArrayStream stream{encoded_extrinsic_};
  auto &&res = codec_->decodeExtrinsic(stream);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value().data(), extrinsic_.data());
}

/**
 * @given predefined instance of Block
 * @when encodeBlock is applied
 * @then expected result obtained
 */
TEST_F(Primitives, encodeBlock) {
  auto &&res = codec_->encodeBlock(block_);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), encoded_block_);
}

/**
 * @given encoded block
 * @when decodeBlock is applied
 * @then decoded instance of Block matches predefined Block instance
 */
TEST_F(Primitives, decodeBlock) {
  ByteArrayStream stream{encoded_block_};
  auto &&res = codec_->decodeBlock(stream);
  ASSERT_TRUE(res);

  auto &&h = res.value().header();

  ASSERT_EQ(h.parentHash(), block_header_.parentHash());
  ASSERT_EQ(h.number(), block_header_.number());
  ASSERT_EQ(h.stateRoot(), block_header_.stateRoot());
  ASSERT_EQ(h.extrinsicsRoot(), block_header_.extrinsicsRoot());
  ASSERT_EQ(h.digest(), block_header_.digest());

  auto &&extrinsics = res.value().extrinsics();

  ASSERT_EQ(extrinsics.size(), 1);
  ASSERT_EQ(extrinsics[0].data(), extrinsic_.data());
}

/// Version

/**
 * @given version instance and predefined match for encoded instance
 * @when codec_->encodeVersion() is applied
 * @then obtained result equal to predefined match
 */
TEST_F(Primitives, encodeVersion) {
  auto &&res = codec_->encodeVersion(version_);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), encoded_version_);
}

/**
 * @given byte array containing encoded version and predefined version
 * @when codec_->decodeVersion() is applied
 * @then obtained result matches predefined version instance
 */
TEST_F(Primitives, decodeVersion) {
  auto stream = ByteArrayStream(encoded_version_);
  auto &&res = codec_->decodeVersion(stream);
  ASSERT_TRUE(res);
  auto &&ver = res.value();
  ASSERT_EQ(ver.specName(), version_.specName());
  ASSERT_EQ(ver.implName(), version_.implName());
  ASSERT_EQ(ver.authoringVersion(), version_.authoringVersion());
  ASSERT_EQ(ver.apis(), version_.apis());
}

/// BlockId

/**
 * @given block id constructed as hash256 and predefined match
 * @when codec_->encodeBlockId() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, encodeBlockIdHash256) {
  auto &&res = codec_->encodeBlockId(block_id_hash_);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), encoded_block_id_hash_);
}

/**
 * @given block id constructed as BlockNumber and predefined match
 * @when codec_->encodeBlockId() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, encodeBlockIdBlockNumber) {
  auto &&res = codec_->encodeBlockId(block_id_number_);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), (encoded_block_id_number_));
}

/**
 * @given byte array containing encoded block id
 * and predefined block id as Hash256
 * @when codec_->decodeBlockId() is applied
 * @thenobtained result matches predefined block id instance
 */
TEST_F(Primitives, decodeBlockIdHash) {
  auto stream = ByteArrayStream(encoded_block_id_hash_);
  auto &&res = codec_->decodeBlockId(stream);
  ASSERT_TRUE(res);
  // ASSERT_EQ has problems with pretty-printing variants
  ASSERT_TRUE(res.value() == block_id_hash_);
}

/**
 * @given byte array containing encoded block id
 * and predefined block id as BlockNumber
 * @when codec_->decodeBlockId() is applied
 * @thenobtained result matches predefined block id instance
 */
TEST_F(Primitives, decodeBlockIdNumber) {
  auto stream = ByteArrayStream(encoded_block_id_number_);
  auto &&res = codec_->decodeBlockId(stream);
  ASSERT_TRUE(res);
  // ASSERT_EQ has problems with pretty-printing variants
  ASSERT_TRUE(res.value() == block_id_number_);
}

/// TransactionValidity

/**
 * @given TransactionValidity instance as Invalid and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, encodeTransactionValidityInvalid) {
  TransactionValidity invalid = Invalid{1};
  auto &&res = codec_->encodeTransactionValidity(invalid);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), (Buffer{0, 1}));
}

/**
 * @given TransactionValidity instance as Unknown and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, encodeTransactionValidityUnknown) {
  TransactionValidity unknown = Unknown{2};
  auto &&res = codec_->encodeTransactionValidity(unknown);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), (Buffer{2, 2}));
}

/**
 * @given TransactionValidity instance as Valid and predefined match
 * @when codec_->encodeTransactionValidity() is applied
 * @then obtained result matches predefined value
 */
TEST_F(Primitives, encodeTransactionValidity) {
  auto &&res = codec_->encodeTransactionValidity(valid_transaction_);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), encoded_valid_transaction_);
}

/**
 * @given byte array containing encoded
 * and TransactionValidity instance as Valid
 * @when codec_->decodeBlockId() is applied
 * @thenobtained result matches predefined block id instance
 */
TEST_F(Primitives, decodeTransactionValidityInvalid) {
  Buffer bytes = {0, 1};
  auto stream = ByteArrayStream(bytes);
  auto &&res = codec_->decodeTransactionValidity(stream);
  ASSERT_TRUE(res);
  TransactionValidity value = res.value();
  kagome::visit_in_place(
      value,                                       // value
      [](Invalid &v) { ASSERT_EQ(v.error_, 1); },  // ok
      [](Unknown &v) { FAIL(); },                  // fail
      [](Valid &v) { FAIL(); });                   // fail
}

/**
 * @given byte array containing encoded
 * and TransactionValidity instance as Valid
 * @when codec_->decodeBlockId() is applied
 * @thenobtained result matches predefined block id instance
 */
TEST_F(Primitives, decodeTransactionValidityUnknown) {
  Buffer bytes = {2, 2};
  auto stream = ByteArrayStream(bytes);
  auto &&res = codec_->decodeTransactionValidity(stream);
  ASSERT_TRUE(res);
  TransactionValidity value = res.value();
  kagome::visit_in_place(
      value,                                       // value
      [](Invalid &v) { FAIL(); },                  // fail
      [](Unknown &v) { ASSERT_EQ(v.error_, 2); },  // ok
      [](Valid &v) { FAIL(); }                     // fail
  );
}

/**
 * @given byte array containing encoded
 * and TransactionValidity instance as Valid
 * @when codec_->decodeBlockId() is applied
 * @thenobtained result matches predefined block id instance
 */
TEST_F(Primitives, decodeTransactionValidityValid) {
  auto stream = ByteArrayStream(encoded_valid_transaction_);
  auto &&res = codec_->decodeTransactionValidity(stream);
  ASSERT_TRUE(res);
  TransactionValidity value = res.value();
  kagome::visit_in_place(
      value,                       // value
      [](Invalid &v) { FAIL(); },  // fail
      [](Unknown &v) { FAIL(); },  // fail
      [this](Valid &v) {           // ok
        auto &valid = valid_transaction_;
        ASSERT_EQ(v.priority_, valid.priority_);
        ASSERT_EQ(v.requires_, valid.requires_);
        ASSERT_EQ(v.provides_, valid.provides_);
        ASSERT_EQ(v.longevity_, valid.longevity_);
      });
}
