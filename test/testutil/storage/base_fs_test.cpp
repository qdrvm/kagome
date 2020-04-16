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

#include "testutil/storage/base_fs_test.hpp"

namespace test {

  void BaseFS_Test::clear() {
    if (fs::exists(base_path)) {
      fs::remove_all(base_path);
    }
  }

  void BaseFS_Test::mkdir() {
    fs::create_directory(base_path);
  }

  std::string BaseFS_Test::getPathString() const {
    return fs::canonical(base_path).string();
  }

  BaseFS_Test::~BaseFS_Test() {
    clear();
  }

  BaseFS_Test::BaseFS_Test(fs::path path) : base_path(std::move(path)) {
    clear();
    mkdir();

    logger = kagome::common::createLogger(getPathString());
    logger->set_level(spdlog::level::debug);
  }

  void BaseFS_Test::SetUp() {
    clear();
    mkdir();
  }

  void BaseFS_Test::TearDown() {
    clear();
  }
}  // namespace test
