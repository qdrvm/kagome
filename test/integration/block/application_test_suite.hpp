/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_INTEGRATION_BLOCK_TESTSUITE_HPP
#define KAGOME_TEST_INTEGRATION_BLOCK_TESTSUITE_HPP

#include <gtest/gtest.h>

#include "injector/application_injector.hpp"

class ApplicationTestSuite : public testing::Test {
 protected:
  auto &getInjector() const {
    static auto injector =
        kagome::injector::makeApplicationInjector("config/polkadot-v06.json",
                                                  "ldb",
                                                  40363,
                                                  40364);
    return injector;
  }
};

#endif // KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP
