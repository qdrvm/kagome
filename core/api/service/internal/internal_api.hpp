/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_INTERNALAPI
#define KAGOME_API_INTERNALAPI

#include <boost/variant.hpp>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"

namespace kagome::api {

  /**
   * @class InternalApi provides doing various action over RPC
   */
  class InternalApi {
   public:
    virtual ~InternalApi() = default;

    virtual outcome::result<void> setLogLevel(const std::string &group,
                                              const std::string &level) = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_INTERNALAPI
