/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <qtils/bytestr.hpp>
#include <qtils/outcome.hpp>

namespace kagome {
  inline outcome::result<void> writeFile(const std::filesystem::path &path,
                                         std::string_view data) {
    std::ofstream file{path, std::ios::binary};
    if (file and file.write(data.data(), data.size()) and file.flush()) {
      return outcome::success();
    }
    return std::errc{errno};
  }

  inline outcome::result<void> writeFile(const std::filesystem::path &path,
                                         qtils::BytesIn data) {
    return writeFile(path, qtils::byte2str(data));
  }

  outcome::result<void> writeFileTmp(const std::filesystem::path &path,
                                     auto &&data) {
    boost::system::error_code ec1;
    auto tmp =
        boost::filesystem::unique_path(path.native() + ".%%%%", ec1).native();
    if (ec1) {
      return ec1;
    }
    OUTCOME_TRY(writeFile(tmp, data));
    std::error_code ec2;
    std::filesystem::rename(tmp, path, ec2);
    if (ec2) {
      return ec2;
    }
    return outcome::success();
  }
}  // namespace kagome
