/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <outcome/outcome.hpp>
#include "primitives/block.hpp"
#include "primitives/impl/scale_codec_impl.hpp"
#include "scale/byte_array_stream.hpp"

using kagome::common::Buffer;
using kagome::scale::ByteArrayStream;

using namespace kagome::primitives;  // NOLINT

/**
 * @class Primitives is a test fixture which contains useful data
 */
class Primitives : public testing::Test {
 public:
  Primitives()
      : header_{{1},   // buffer: parent_hash
                2,     // number: number
                {3},   // buffer: state_root
                {4},   // buffer: extrinsic root
                {5}},  // buffer: digest
        extrinsic_{{12, 1, 2, 3}},
        // sequence of 3 bytes: 1, 2, 3; 12 == (3 << 2)

        header_match_{
            4, 1,                    // {1}
            2, 0, 0, 0, 0, 0, 0, 0,  // 2 as uint64
            4, 3,                    // {3}
            4, 4,                    // {4}
            4, 5                     // {5}
        },
        extrinsic_match_{12, 1, 2, 3},
        block_{header_, {extrinsic_}},
        block_match_{4,  1,                    // {1}
                     2,  0, 0, 0, 0, 0, 0, 0,  // 2 as uint64
                     4,  3,                    // {3}
                     4,  4,                    // {4}
                     4,  5,                    // {5}
                     4,                        // (1 << 2) number of extrinsics
                     12, 1, 2, 3}              // extrinsic itself {1, 2, 3}
  {
    codec_ = std::make_shared<ScaleCodecImpl>();
  }

  const BlockHeader &header() const {
    return header_;
  }

  const Buffer &headerMatch() const {
    return header_match_;
  }

  const Extrinsic &extrinsic() const {
    return extrinsic_;
  }

  const Buffer &extrinsicMatch() const {
    return extrinsic_match_;
  }

  const Block &block() const {
    return block_;
  }

  const Buffer &blockMatch() const {
    return block_match_;
  }

  auto codec() const {
    return codec_;
  }

 private:
  BlockHeader header_;
  Extrinsic extrinsic_;
  Buffer header_match_;
  Buffer extrinsic_match_;
  Block block_;
  Buffer block_match_;

  std::shared_ptr<ScaleCodec> codec_;
};

/**
 * @given predefined block header
 * @when encodeBlockHeader is applied
 * @then expected result obtained
 */
TEST_F(Primitives, encodeBlockHeader) {
  auto &&res = codec()->encodeBlockHeader(header());

  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), headerMatch());
}

/**
 * @given scale-encoded block header
 * @when decodeBlockHeader is applied
 * @then decoded instance of BlockHeader matches predefined block header
 */
TEST_F(Primitives, decodeBlockHeader) {
  ByteArrayStream stream{headerMatch()};

  auto &&res = codec()->decodeBlockHeader(stream);
  ASSERT_TRUE(res);

  auto &&val = res.value();

  ASSERT_EQ(val.parentHash(), header().parentHash());
  ASSERT_EQ(val.number(), header().number());
  ASSERT_EQ(val.stateRoot(), header().stateRoot());
  ASSERT_EQ(val.extrinsicsRoot(), header().extrinsicsRoot());
  ASSERT_EQ(val.digest(), header().digest());
}

/**
 * @given predefined extrinsic containing sequence {12, 1, 2, 3}
 * @when encodeExtrinsic is applied
 * @then the same expected buffer obtained {12, 1, 2, 3}
 */
TEST_F(Primitives, encodeExtrinsic) {
  auto &&res = codec()->encodeExtrinsic(extrinsic());
  ASSERT_TRUE(res);

  auto &&val = res.value();
  ASSERT_EQ(val, extrinsicMatch());
}

/**
 * @given encoded extrinsic
 * @when decodeExtrinsic is applied
 * @then decoded instance of Extrinsic matches predefined block header
 */
TEST_F(Primitives, decodeExtrinsic) {
  ByteArrayStream stream{extrinsicMatch()};
  auto &&res = codec()->decodeExtrinsic(stream);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value().data(), extrinsic().data());
}

/**
 * @given predefined instance of Block
 * @when encodeBlock is applied
 * @then expected result obtained
 */
TEST_F(Primitives, encodeBlock) {
  auto &&res = codec()->encodeBlock(block());
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value(), blockMatch());
}

/**
 * @given encoded block
 * @when decodeBlock is applied
 * @then decoded instance of Block matches predefined Block instance
 */
TEST_F(Primitives, decodeBlock) {
  ByteArrayStream stream{blockMatch()};
  auto &&res = codec()->decodeBlock(stream);
  ASSERT_TRUE(res);

  auto &&h = res.value().header();

  ASSERT_EQ(h.parentHash(), header().parentHash());
  ASSERT_EQ(h.number(), header().number());
  ASSERT_EQ(h.stateRoot(), header().stateRoot());
  ASSERT_EQ(h.extrinsicsRoot(), header().extrinsicsRoot());
  ASSERT_EQ(h.digest(), header().digest());

  auto &&extrinsics = res.value().extrinsics();

  ASSERT_EQ(extrinsics.size(), 1);
  ASSERT_EQ(extrinsics[0].data(), extrinsic().data());
}
