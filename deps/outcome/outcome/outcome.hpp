/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OUTCOME_HPP
#define KAGOME_OUTCOME_HPP

#include <outcome/outcome-external.hpp>
#include <outcome/outcome-register.hpp>

namespace outcome = OUTCOME_V2_NAMESPACE; // required to be here

/**
 * @brief outcome::result<T> is a type which can be used as "value or error" type.
 *
 * @example see {@file test/deps/outcome_test.cpp}
 *
 * @code
 * enum ConversionErrc {
 *   Success = 0,                                    // do not use 0 as error code
 *   EmptyString,                                    // define list of errors. Select meaningful names.
 *   IllegalChar,
 *   TooLong
 * }
 *
 * // clang-format off                               // disable clang-format, as it makes macro ugly
 * OUTCOME_REGISTER_ERROR(                           // call registration macro
 *   ConversionErrc                                  // provide name of the enum
 *   ,                                               // PUT EXACTLY ONE COMMA
 *   (ConversionErrc::Success, "success")            // then, provide list of tuples (case, "message")
 *   (ConversionErrc::EmptyString, "empty string")   // then, provide list of tuples (case, "message")
 *   (ConversionErrc::IllegalChar, "illegal char")   // then, provide list of tuples (case, "message")
 *   (ConversionErrc::TooLong, "too long")           // then, provide list of tuples (case, "message")
 * );                                                // close macro and put ;
 * // clang-format on                                // enable clang-format
 * @nocode
 */

#endif //KAGOME_OUTCOME_HPP
