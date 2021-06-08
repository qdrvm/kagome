/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_CORE_API_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_CORE_API_PROVIDER_HPP

#include "runtime/core.hpp"

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::runtime {

  class CoreApiProvider {
   public:
    virtual ~CoreApiProvider() = default;

    virtual std::unique_ptr<Core> makeCoreApi(
        std::shared_ptr<const crypto::Hasher> hasher,
        gsl::span<uint8_t> runtime_code) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_CORE_API_PROVIDER_HPP
