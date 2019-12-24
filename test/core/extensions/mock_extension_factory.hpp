/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_EXTENSIONS_MOCK_EXTENSION_FACTORY_HPP
#define KAGOME_TEST_CORE_EXTENSIONS_MOCK_EXTENSION_FACTORY_HPP

#include "extensions/extension_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::extensions {

  class MockExtensionFactory : public ExtensionFactory {
   public:
    MOCK_CONST_METHOD1(
        createExtension,
        std::shared_ptr<Extension>(std::shared_ptr<runtime::WasmMemory>));
  };

}  // namespace kagome::extensions

#endif  // KAGOME_TEST_CORE_EXTENSIONS_MOCK_EXTENSION_FACTORY_HPP
