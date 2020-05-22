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
    static auto injector = kagome::injector::makeApplicationInjector(
        (boost::filesystem::current_path().parent_path().parent_path()/"node"/"config"/"localchain.json").native(),
        (boost::filesystem::temp_directory_path()/boost::filesystem::unique_path()).native(),
        {},
        {});
    return injector;
  }
};

#endif  // KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP
