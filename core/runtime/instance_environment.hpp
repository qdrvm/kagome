/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <memory>

namespace kagome::host_api {
  class HostApi;
}

namespace kagome::runtime {
  class TrieStorageProvider;
  class MemoryProvider;

  struct InstanceEnvironment {
    InstanceEnvironment(const InstanceEnvironment &) = delete;
    InstanceEnvironment &operator=(const InstanceEnvironment &) = delete;

    InstanceEnvironment(
        std::shared_ptr<MemoryProvider> memory_provider,
        std::shared_ptr<TrieStorageProvider> storage_provider,
        std::shared_ptr<host_api::HostApi> host_api,
        std::function<void(InstanceEnvironment &)> on_destruction)
        : memory_provider(std::move(memory_provider)),
          storage_provider(std::move(storage_provider)),
          host_api(std::move(host_api)),
          on_destruction(std::move(on_destruction)) {}

    InstanceEnvironment(InstanceEnvironment &&e)
        : memory_provider(std::move(e.memory_provider)),
          storage_provider(std::move(e.storage_provider)),
          host_api(std::move(e.host_api)),
          on_destruction(std::move(e.on_destruction)) {
      e.on_destruction = {};
    }
    InstanceEnvironment &operator=(InstanceEnvironment &&) = default;

    ~InstanceEnvironment() {
      if (on_destruction) {
        on_destruction(*this);
      }
    }

    std::shared_ptr<MemoryProvider> memory_provider;
    std::shared_ptr<TrieStorageProvider> storage_provider;
    std::shared_ptr<host_api::HostApi> host_api;
    std::function<void(InstanceEnvironment &)> on_destruction;
  };

}  // namespace kagome::runtime
