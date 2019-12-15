/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "injector/application_injector.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::injector, InjectorError, e) {
  using E = kagome::injector::InjectorError;
  switch (e) {
    case E::INDEX_OUT_OF_BOUND:
      return "index out of bound";
  }
  return "Unknown error";
}
