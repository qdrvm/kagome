/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_FS_TEST_HPP
#define KAGOME_BASE_FS_TEST_HPP

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "log/logger.hpp"

// intentionally here, so users can use fs shortcut
namespace fs = boost::filesystem;

namespace test {

  /**
   * @brief Base test, which involves filesystem. Can be created with given
   * path. Clears path before test and after test.
   */
  struct BaseFS_Test : public ::testing::Test {
    // not explicit, intentionally
    BaseFS_Test(fs::path path);

    void clear();

    void mkdir();

    std::string getPathString() const;

    ~BaseFS_Test() override;

    void TearDown() override;

    void SetUp() override;

   protected:
    fs::path base_path;
    kagome::log::Logger logger;
  };

}  // namespace test

#endif  // KAGOME_BASE_FS_TEST_HPP
