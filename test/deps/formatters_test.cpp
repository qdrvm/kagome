/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "outcome/outcome.hpp"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <string>

#include "blockchain/genesis_block_hash.hpp"
#include "common/buffer.hpp"
#include "common/hexutil.hpp"
#include "primitives/common.hpp"

TEST(Formatter, X) {
  {
    kagome::common::Buffer x;
    std::ignore = fmt::format("{}", kagome::common::hex_lower(x));
  }
  {
    std::shared_ptr<kagome::common::Buffer> x;
    std::ignore = fmt::format("{}", static_cast<void *>(x.get()));
  }
  {
    kagome::primitives::BlockHash x;
    std::ignore = fmt::format("{}", x);
  }
}
