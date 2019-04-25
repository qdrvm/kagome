/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OUTCOME_HPP
#define KAGOME_OUTCOME_HPP

#include <outcome/outcome-external.hpp>
#include <outcome/outcome-register.hpp>
#include <outcome/outcome-util.hpp>

/**
 * __cpp_sized_deallocation macro interferes with protobuf generated files
 * and makes them uncompilable by means of clang
 * since it is not necessary outside of outcome internals
 * it can be safely undefined
 */
#undef __cpp_sized_deallocation

namespace outcome = OUTCOME_V2_NAMESPACE;  // required to be here

// @see /docs/result.md

#endif  // KAGOME_OUTCOME_HPP
