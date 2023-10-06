/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/outcome/dummy_error.hpp"
#include "outcome/outcome.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(testutil, DummyError, e) {
  using testutil::DummyError;
  switch (e) {
    case DummyError::ERROR:
      return "dummy error";
    case DummyError::ERROR_2:
      return "dummy error #2";
    case DummyError::ERROR_3:
      return "dummy error #3";
    case DummyError::ERROR_4:
      return "dummy error #4";
    case DummyError::ERROR_5:
      return "dummy error #5";
  }
  return "unknown (DummyError) error";
}
