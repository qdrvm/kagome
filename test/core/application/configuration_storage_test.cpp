/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "application/impl/configuration_storage_impl.hpp"

using kagome::application::ConfigurationStorageImpl;
using kagome::application::KagomeConfig;
using kagome::primitives::Block;

/**
 * @given kagome config structure
 * @when creating a configuration storage with the given config
 * @then the content of the storage matches the original config
 */
TEST(ConfigurationStorageTest, MatchesConfig) {
  KagomeConfig c;
  c.genesis.header.number = 42;
  ConfigurationStorageImpl s(c);
  ASSERT_EQ(s.getGenesis().header.number, c.genesis.header.number);
}
