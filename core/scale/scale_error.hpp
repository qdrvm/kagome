/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_ERROR_HPP
#define KAGOME_SCALE_ERROR_HPP

#include <outcome/outcome.hpp>
#include "scale/types.hpp"

// TODO(yuraz): move scale error to scale library
// TODO(yuraz): under the same task id after actual task is done

OUTCOME_HPP_DECLARE_ERROR(kagome::common::scale, EncodeError)
OUTCOME_HPP_DECLARE_ERROR(kagome::common::scale, DecodeError)

#endif  // KAGOME_SCALE_ERROR_HPP
