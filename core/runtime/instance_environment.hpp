/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_INSTANCE_ENVIRONMENT_HPP
#define KAGOME_CORE_RUNTIME_INSTANCE_ENVIRONMENT_HPP

#include <memory>

namespace kagome::host_api {
  class HostApi;
}

namespace kagome::runtime {
  class TrieStorageProvider;
  class MemoryProvider;

  struct InstanceEnvironment {
    std::shared_ptr<MemoryProvider> memory_provider;
    std::shared_ptr<TrieStorageProvider> storage_provider;
    std::shared_ptr<host_api::HostApi> host_api;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_INSTANCE_ENVIRONMENT_HPP
