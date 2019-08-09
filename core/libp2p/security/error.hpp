/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SECURITY_ERROR_HPP
#define KAGOME_SECURITY_ERROR_HPP

#include <outcome/outcome.hpp>

namespace libp2p::security {

  enum SecurityError { SUCCESS = 0, AUTHENTICATION_ERROR };

}

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::SecurityError);

#endif  // KAGOME_SECURITY_ERROR_HPP
