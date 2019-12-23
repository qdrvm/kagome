/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP
#define KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP

#include "extensions/extension.hpp"

namespace kagome::extensions {

  class ExtensionFactory {
   public:
    virtual ~ExtensionFactory() = default;

    virtual std::shared_ptr<Extension> createExtension(
        std::shared_ptr<runtime::WasmMemory> memory) const = 0;
  };

}  // namespace kagome::extensions

#endif  // KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP
