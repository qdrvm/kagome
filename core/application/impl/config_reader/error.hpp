/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP
#define KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::application {

  /**
   * Codes for errors that originate in configuration readers
   */
  enum class ConfigReaderError { MISSING_ENTRY = 1, PARSER_ERROR };

}  // namespace kagome::application

OUTCOME_HPP_DECLARE_ERROR(kagome::application, ConfigReaderError);

#endif  // KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP
