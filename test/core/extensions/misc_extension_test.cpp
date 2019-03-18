/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "extensions/impl/misc_extension.hpp"

TEST(MiscExt, Init) {
  kagome::extensions::MiscExtension m;
  ASSERT_EQ(m.ext_chain_id(), 42);

  kagome::extensions::MiscExtension m2(34);
  ASSERT_EQ(m2.ext_chain_id(), 34);
}
