/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_GC_COMPARTMENT_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_GC_COMPARTMENT_HPP

namespace WAVM::Runtime {
  struct Compartment;
}  // namespace WAVM::Runtime

WAVM::Runtime::Compartment* getCompartment();

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_GC_COMPARTMENT_HPP
