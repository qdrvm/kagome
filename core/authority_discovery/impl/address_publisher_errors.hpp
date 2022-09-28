/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_ERRORS
#define KAGOME_ADAPTERS_ERRORS

#include "outcome/outcome.hpp"

namespace kagome::authority_discovery {
  /**
   * @brief interface adapters errors
   */
  enum class AddressPublisherError : int {
    NO_GRAND_KEY = 1,
    NO_AUDI_KEY,
    NO_AUTHORITY,
    WRONG_KEY_TYPE,
  };

}  // namespace kagome::authority_discovery

OUTCOME_HPP_DECLARE_ERROR(kagome::authority_discovery, AddressPublisherError)

#endif  // KAGOME_ADAPTERS_ERRORS
