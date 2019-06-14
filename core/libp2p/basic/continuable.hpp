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

#include <continuable/continuable.hpp>

namespace libp2p::basic {
//  template <typename ContValueType>
//  cti::continuable<ContValueType> makeErrorContinuable(std::error_code ec) {
//    return cti::make_continuable<ContValueType>(
//        [ec](auto &&promise) { promise.set_exception(ec); });
//  }
//
//  template <>
//  cti::continuable<> makeErrorContinuable(std::error_code ec) {
//    return cti::make_continuable<void>(
//        [ec](auto &&promise) { promise.set_exception(ec); });
//  }
}  // namespace libp2p::basic

#endif  // KAGOME_CONTINUABLE_HPP
