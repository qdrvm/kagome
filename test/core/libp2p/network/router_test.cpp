/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/impl/router_impl.hpp"

#include <gtest/gtest.h>

using namespace libp2p::network;

class RouterTest : public ::testing::Test {
 public:
  std::shared_ptr<Router> router_ = std::make_shared<RouterImpl>();
};

