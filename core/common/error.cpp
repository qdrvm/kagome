/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome, Error, e) {
  using kagome::Error;
  switch (e) {
    case Error::NO_ERROR:
      return "No error case";
    case Error::OTHER_ERROR:
      return "Some error has happened";
    case Error::OTHER_EXCEPTION:
      return "Some exception has caught";
  }
  return "unknown error (kagome::Error)";
}
