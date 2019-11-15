/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEY_STORAGE_ERROR_HPP
#define KAGOME_KEY_STORAGE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::application {

  /**
   * Error codes for key storage and its underlying components, e. g. crypto key
   * reader
   */
  enum class KeyStorageError {
    FILE_READ_ERROR = 1,
    PRIVATE_KEY_READ_ERROR,
    MALFORMED_KEY,
    UNSUPPORTED_KEY_TYPE
  };

}  // namespace kagome::application

OUTCOME_HPP_DECLARE_ERROR(kagome::application, KeyStorageError);

#endif  // KAGOME_KEY_STORAGE_ERROR_HPP
