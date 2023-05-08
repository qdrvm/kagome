/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/util.hpp"

#define UNWRAP_ERROR_CODE(ec)      \
  {                                \
    if (ec) {                      \
      return outcome::failure(ec); \
    }                              \
  }

OUTCOME_CPP_DEFINE_CATEGORY(kagome::application::util, Error, e) {
  using E = kagome::application::util::Error;
  switch (e) {
    case E::FAILED_TO_CREATE_DIR:
      return "Failed to create directory";
    case E::NOT_A_DIR:
      return "File already exists, but it's not a directory";
  }
  return "Unknown application::util error";
}

namespace kagome::application::util {

  namespace fs = filesystem;

  outcome::result<void> init_directory(const fs::path &path) {
    // init chain directory in base path
    std::error_code ec{};
    if (not fs::exists(path, ec)) {
      UNWRAP_ERROR_CODE(ec)
      if (not fs::create_directory(path, ec)) {
        return Error::FAILED_TO_CREATE_DIR;
      }
      UNWRAP_ERROR_CODE(ec)
    } else {
      if (not fs::is_directory(path, ec)) {
        return Error::NOT_A_DIR;
      }
      UNWRAP_ERROR_CODE(ec)
    }
    return outcome::success();
  }

}  // namespace kagome::application::util
