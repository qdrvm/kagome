/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_MODULE_MODULE_FACTORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_MODULE_MODULE_FACTORY_IMPL_HPP

#include "runtime/module_factory.hpp"

namespace kagome::runtime {
  class TrieStorageProvider;
}

namespace kagome::runtime::binaryen {

  class RuntimeExternalInterface;

  class ModuleFactoryImpl final : public ModuleFactory {
   public:
    enum class Error {
      EXTERNAL_INTERFACE_OUTDATED = 1,
    };

    ModuleFactoryImpl(std::weak_ptr<RuntimeExternalInterface> rei,
                      std::shared_ptr<TrieStorageProvider> storage_provider);

    outcome::result<std::unique_ptr<Module>> make(
        gsl::span<const uint8_t> code) const override;

    void setExternalInterface(std::weak_ptr<RuntimeExternalInterface> rei);

   private:
    std::weak_ptr<RuntimeExternalInterface> rei_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, ModuleFactoryImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_MODULE_MODULE_FACTORY_IMPL_HPP
