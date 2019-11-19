/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "application/impl/configuration_storage_impl.hpp"
#include "core/application/example_config.hpp"

using kagome::application::ConfigurationStorageImpl;
using kagome::application::KagomeConfig;
using kagome::primitives::Block;

/**
 * @given kagome config structure
 * @when creating a configuration storage with the given config
 * @then the content of the storage matches the original config
 */
TEST(ConfigurationStorageTest, MatchesConfig) {
  KagomeConfig conf = test::application::getExampleConfig();
  ConfigurationStorageImpl s(conf);
  ASSERT_EQ(s.getGenesis(), conf.genesis);
  ASSERT_EQ(s.getExtrinsicApiPort(), conf.api_ports.extrinsic_api_port);
  ASSERT_EQ(s.getAuthorities(), conf.authorities);
  ASSERT_EQ(s.getPeersInfo(), conf.peers_info);
  ASSERT_EQ(s.getSessionKeys(), conf.session_keys);
}
