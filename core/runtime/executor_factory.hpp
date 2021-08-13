/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_EXECUTOR_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_EXECUTOR_FACTORY_HPP

#include <memory>
#include <vector>

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::runtime {

  class Executor;

  /**
   * A factory for Runtime Executor, used where an isolated runtime environment
   * is required
   */
  class ExecutorFactory {
   public:
    virtual ~ExecutorFactory() = default;

    [[nodiscard]] virtual std::unique_ptr<Executor> make(
        std::shared_ptr<const crypto::Hasher> hasher,
        const std::vector<uint8_t> &runtime_code) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_EXECUTOR_FACTORY_HPP
