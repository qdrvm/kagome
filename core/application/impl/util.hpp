/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_UTIL_HPP
#define KAGOME_APPLICATION_UTIL_HPP

#include <boost/filesystem.hpp>

#include "outcome/outcome.hpp"

namespace kagome::application::util {

  enum class Error {
    FAILED_TO_CREATE_DIR = 1,
    NOT_A_DIR
  };

  outcome::result<void> init_directory(const boost::filesystem::path& path);

}

OUTCOME_HPP_DECLARE_ERROR(kagome::application::util, Error);

#endif  // KAGOME_UTIL_HPP
