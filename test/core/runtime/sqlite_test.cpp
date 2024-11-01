/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "common/buffer.hpp"

#include <gtest/gtest.h>
#include <sqlite_modern_cpp.h>

#include <utility>

struct Extrin {
  std::string name;
  std::string data;
  std::string root;
};

std::optional<std::string> db_path() {
  const auto *path = std::getenv("EXTS_DB_PATH");
  if (path != nullptr) {
    return std::string(path);
  }
  return std::nullopt;
}

std::vector<Extrin> read_extrinsics(const std::string db_path) {
  sqlite::database db(db_path);
  std::vector<Extrin> exts;
  db << "SELECT name,ext,root FROM exts ;" >>
      [&](std::string name, std::string ext, std::string root) {
        exts.emplace_back(Extrin{.name = std::move(name),
                                 .data = std::move(ext),
                                 .root = std::move(root)});
      };
  return exts;
}

using kagome::Buffer;  // NOLINT

TEST(SQL, First) {
  std::cout << db_path().value() << std::endl;
  auto exts = read_extrinsics(db_path().value());
  std::cout << "done" << std::endl;
}
