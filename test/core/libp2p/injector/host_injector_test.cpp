/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/injector/host_injector.hpp"

#include <gtest/gtest.h>

using namespace libp2p;
using namespace injector;

TEST(HostInjector, Default){
  auto injector = makeHostInjector();

  auto unique = injector.create<std::unique_ptr<Host>>();
  auto shared = injector.create<std::shared_ptr<Host>>();

  ASSERT_NE(unique, nullptr);
  ASSERT_NE(shared, nullptr);
}
