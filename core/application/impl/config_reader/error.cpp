/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::application,
                            ConfigReaderError,
                            e) {
  using E = kagome::application::ConfigReaderError;
  switch (e) {
    case E::MISSING_ENTRY:
      return "A required entry is missing in the provided config file";
    case E::FILE_NOT_FOUND:
      return "Config file not found";
  }
  return "Unknown error";
}
