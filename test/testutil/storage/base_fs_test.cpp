/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/storage/base_fs_test.hpp"

#include "log/logger.hpp"
#include "testutil/prepare_loggers.hpp"

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
    logger = kagome::log::createLogger(getPathString(), "testing");
    logger->setLevel(kagome::log::Level::DEBUG);
  }

  void BaseFS_Test::SetUp() {
    clear();
    mkdir();
  }

  void BaseFS_Test::TearDown() {
    clear();
  }
}  // namespace test
