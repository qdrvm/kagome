/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/key_storage_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::application, KeyStorageError, e) {
  using E = kagome::application::KeyStorageError;
  switch(e) {
    case E::FILE_READ_ERROR:
      return "Error occured when reading a file";
    case E::MALFORMED_KEY:
      return "The read key is invalid and cannot be processed";
    case E::PRIVATE_KEY_READ_ERROR:
      return "Error reading a private key from a file";
    case E::UNSUPPORTED_KEY_TYPE:
      return "The provided key type is not supported";
  }
  return "Unknown error";
}
