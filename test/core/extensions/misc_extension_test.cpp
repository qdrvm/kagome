/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/misc_extension.hpp"
#include <gtest/gtest.h>

/**
 * @given a chain id
 * @when initializing misc extention
 * @then ext_chain_id return the chain id
 */
TEST(MiscExt, Init) {
  kagome::extensions::MiscExtension m{42, nullptr, nullptr};
  ASSERT_EQ(m.ext_chain_id(), 42);

  kagome::extensions::MiscExtension m2(34, nullptr, nullptr);
  ASSERT_EQ(m2.ext_chain_id(), 34);
}

/**
 * @given a chain id
 * @when initializing misc extention
 * @then ext_chain_id return the chain id
 */
TEST(MiscExt, CoreVersion) {
  kagome::extensions::MiscExtension m{42, nullptr, nullptr};
  ASSERT_EQ(m.ext_chain_id(), 42);

  kagome::extensions::MiscExtension m2(34, nullptr, nullptr);
  ASSERT_EQ(m2.ext_chain_id(), 34);
}
