/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_EXTENSIONS_MOCK_HOST_API_FACTORY_HPP
#define KAGOME_TEST_CORE_EXTENSIONS_MOCK_HOST_API_FACTORY_HPP

#include "host_api/host_api_factory.hpp"

#include <gmock/gmock.h>

// some GMock internals dislike forward declaration of CoreFactory
#include "runtime/binaryen/core_factory.hpp"

namespace kagome::host_api {

  class HostApiFactoryMock : public HostApiFactory {
   public:
    MOCK_CONST_METHOD3(
        make,
        std::unique_ptr<HostApi>(
            std::shared_ptr<const runtime::CoreApiFactory> core_provider,
            std::shared_ptr<const runtime::MemoryProvider> memory_provider,
            std::shared_ptr<runtime::TrieStorageProvider> storage));
  };

}  // namespace kagome::host_api

#endif  // KAGOME_TEST_CORE_EXTENSIONS_MOCK_HOST_API_FACTORY_HPP
