/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "libp2p/multi/utils/ip_v4_validator.hpp"

using libp2p::multi::IPv4Validator;

TEST(IPv4Validator, validate) {
  ASSERT_TRUE(IPv4Validator::isValidIp("127.0.0.1"));
  ASSERT_TRUE(IPv4Validator::isValidIp("8.8.8.8"));
  ASSERT_TRUE(IPv4Validator::isValidIp("255.255.0.255"));

  ASSERT_FALSE(IPv4Validator::isValidIp("256.255.0.255"));
  ASSERT_FALSE(IPv4Validator::isValidIp(".255.255.0.255"));
  ASSERT_FALSE(IPv4Validator::isValidIp("255..255.0.255"));
  ASSERT_FALSE(IPv4Validator::isValidIp("255.255.0.255."));
  ASSERT_FALSE(IPv4Validator::isValidIp("255.255.4f4.255"));
  ASSERT_FALSE(IPv4Validator::isValidIp("255.255.4f4."));
  ASSERT_FALSE(IPv4Validator::isValidIp("255.255.4.."));
}
