/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP
#define KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::application {

  enum class ConfigReaderError { MISSING_ENTRY = 1, FILE_NOT_FOUND };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::application, ConfigReaderError);

#endif  // KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP
