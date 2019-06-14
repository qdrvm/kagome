/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONTINUABLE_HPP
#define KAGOME_CONTINUABLE_HPP

#include <system_error>

// disable exceptions
#define CONTINUABLE_WITH_NO_EXCEPTIONS

// use std::error_code instead of std::error_condition
#define CONTINUABLE_WITH_CUSTOM_ERROR_TYPE std::error_code

// create a failure continuable with std::error_code
#define ERROR_CONTINUABLE(e) \
  cti::make_exceptional_continuable<std::error_code>(e)

#include <continuable/continuable.hpp>

#endif  // KAGOME_CONTINUABLE_HPP
