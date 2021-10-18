/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "runtime/common/uncompress_code_if_needed.hpp"

#include <gtest/gtest.h>

#include "testutil/prepare_loggers.hpp"

using namespace kagome;   // NOLINT
using namespace runtime;  // NOLINT

class UncompressCodeIfNeeded : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }
};

TEST_F(UncompressCodeIfNeeded, NotOkSize) {
  common::Buffer buf(5, 'a');
  common::Buffer res;
  std::ignore = uncompressCodeIfNeeded(buf, res);
  ASSERT_EQ(res, buf);
}

TEST_F(UncompressCodeIfNeeded, NotNeeded) {
  common::Buffer buf(9, 'a');
  common::Buffer res;
  std::ignore = uncompressCodeIfNeeded(buf, res);
  ASSERT_EQ(res, buf);
}

TEST_F(UncompressCodeIfNeeded, NotNeeded2) {
  common::Buffer buf({0x52, 0xBC, 0x53, 0x76, 0x46, 0xDB, 0x8E, 0x06, 0xFF});
  common::Buffer res({0xAA});
  std::ignore = uncompressCodeIfNeeded(buf, res);
  ASSERT_EQ(res, buf);
}

TEST_F(UncompressCodeIfNeeded, UncompressFail) {
  common::Buffer buf({0x52, 0xBC, 0x53, 0x76, 0x46, 0xDB, 0x8E, 0x05, 0xFF});
  common::Buffer res({0xAA});
  std::ignore = uncompressCodeIfNeeded(buf, res);
  ASSERT_EQ(res, common::Buffer({0xAA}));
}

TEST_F(UncompressCodeIfNeeded, UncompressSucceed) {
  auto buf =
      common::Buffer::fromHex("52BC537646DB8E0528B52FFD200421000062616265")
          .value();
  common::Buffer res;
  std::ignore = uncompressCodeIfNeeded(buf, res);
  ASSERT_EQ(res, common::Buffer({'b', 'a', 'b', 'e'}));
}
