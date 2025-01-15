/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/operations.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <qtils/bytestr.hpp>
#include <qtils/option_take.hpp>
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

  /**
   * Wrapper to generate tmp file name and rename it later.
   * Concurrent writers would corrupt file, so they write to tmp file first, and
   * atomically rename it after file is completely written.
   */
  struct TmpFile {
    /**
     * Generate tmp file name for path.
     * Tmp file is created in same directory as target path,
     * to avoid `EXDEV` error from `rename`.
     */
    static outcome::result<TmpFile> make(std::filesystem::path path) {
      boost::system::error_code ec;
      auto tmp =
          boost::filesystem::unique_path(path.native() + ".%%%%", ec).native();
      if (ec) {
        return ec;
      }
      return TmpFile{std::move(path), std::move(tmp)};
    }

    TmpFile(std::filesystem::path target, std::filesystem::path tmp)
        : target{std::move(target)}, tmp{std::move(tmp)} {}

    /**
     * Get current file path.
     */
    std::filesystem::path path() const {
      return tmp.value_or(target);
    }

    /**
     * Rename file to target name.
     */
    outcome::result<void> rename() {
      if (auto tmp = qtils::optionTake(this->tmp)) {
        std::error_code ec;
        std::filesystem::rename(*tmp, target, ec);
        if (ec) {
          return ec;
        }
      }
      return outcome::success();
    }

    std::filesystem::path target;
    std::optional<std::filesystem::path> tmp;
  };

  outcome::result<void> writeFileTmp(const std::filesystem::path &path,
                                     auto &&data) {
    OUTCOME_TRY(tmp, TmpFile::make(path));
    OUTCOME_TRY(writeFile(tmp.path(), data));
    OUTCOME_TRY(tmp.rename());
    return outcome::success();
  }
}  // namespace kagome
