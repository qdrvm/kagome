/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/module_factory_impl.hpp"

#include "runtime/binaryen/module/module_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            ModuleFactoryImpl::Error,
                            e) {
  using E = kagome::runtime::binaryen::ModuleFactoryImpl::Error;
  switch (e) {
    case E::EXTERNAL_INTERFACE_OUTDATED:
      return "Attempt to use a destroyed external interface";
  }
  return "Unknown error in binaryen's ModuleFactory";
}

namespace kagome::runtime::binaryen {

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::weak_ptr<RuntimeExternalInterface> rei,
      std::shared_ptr<TrieStorageProvider> storage_provider)
      : rei_{std::move(rei)}, storage_provider_{std::move(storage_provider)} {
    BOOST_ASSERT(rei_.lock() != nullptr);
    BOOST_ASSERT(storage_provider_ != nullptr);
  }

  outcome::result<std::unique_ptr<Module>> ModuleFactoryImpl::make(
      gsl::span<const uint8_t> code) const {
    if (rei_.lock() == nullptr) {
      return Error::EXTERNAL_INTERFACE_OUTDATED;
    }
    std::vector<uint8_t> code_vec{code.begin(), code.end()};
    auto res = ModuleImpl::createFromCode(code_vec, storage_provider_, rei_.lock());
    if (res.has_value()) {
      return std::unique_ptr<Module>(std::move(res.value()));
    }
    return res.error();
  }

  void ModuleFactoryImpl::setExternalInterface(
      std::weak_ptr<RuntimeExternalInterface> rei) {
    rei_ = std::move(rei);
  }

}  // namespace kagome::runtime::binaryen
