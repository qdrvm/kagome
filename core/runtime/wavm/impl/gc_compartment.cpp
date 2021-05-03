/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gc_compartment.hpp"

#include <WAVM/Runtime/Runtime.h>

WAVM::Runtime::Compartment *getCompartment() {
  thread_local WAVM::Runtime::Compartment *compartment{
      WAVM::Runtime::createCompartment("Global Compartment")};
  return compartment;
}
