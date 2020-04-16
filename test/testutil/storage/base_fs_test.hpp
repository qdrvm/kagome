/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_BASE_FS_TEST_HPP
#define KAGOME_BASE_FS_TEST_HPP

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "common/logger.hpp"

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
    kagome::common::Logger logger;
  };

}  // namespace test

#endif  // KAGOME_BASE_FS_TEST_HPP
