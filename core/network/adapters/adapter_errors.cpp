/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/adapters/adapter_errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, AdaptersError, e) {
  using E = kagome::network::AdaptersError;
  switch (e) {
    case E::EMPTY_DATA:
      return "No data in buffer.";
    case E::DATA_SIZE_CORRUPTED:
      return "UVAR size does not equal to data size in buffer.";
    case E::PARSE_FAILED:
      return "Got an error while parsing.";
    case E::UNEXPECTED_VARIANT:
      return "Unexpected variant type.";
    case E::CAST_FAILED:
      return "Got an error while casting.";
  }
  return "unknown error (invalid AdaptersError)";
}
