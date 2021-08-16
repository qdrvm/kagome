/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTENSIONS_HOST_API_FACTORY_HPP
#define KAGOME_CORE_EXTENSIONS_HOST_API_FACTORY_HPP

#include "host_api/host_api.hpp"

namespace kagome::runtime {
  class CoreApiFactory;
  class TrieStorageProvider;
  class MemoryProvider;
}  // namespace kagome::runtime

namespace kagome::host_api {

  /**
   * Creates extension containing provided wasm memory
   */
  class HostApiFactory {
   public:
    virtual ~HostApiFactory() = default;

    /**
     * Takes \param memory and creates \return extension using this memory
     */
    virtual std::unique_ptr<HostApi> make(
        std::shared_ptr<const runtime::CoreApiFactory> core_provider,
        std::shared_ptr<const runtime::MemoryProvider> memory_provider,
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider)
        const = 0;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_CORE_EXTENSIONS_HOST_API_FACTORY_HPP
