/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP

#include "runtime/module_instance.hpp"

#include <string_view>

#include <WAVM/Runtime/Runtime.h>
#include <boost/optional.hpp>

#include "log/logger.hpp"
#include "runtime/ptr_size.hpp"

namespace WAVM {
  namespace Runtime {
    struct Instance;
  }  // namespace Runtime
}  // namespace WAVM

namespace kagome::runtime::wavm {

  class CompartmentWrapper;

  class ModuleInstance : public runtime::ModuleInstance {
   public:
    enum class Error {
      FUNC_NOT_FOUND = 1,
      WRONG_ARG_COUNT,
      EXECUTION_ERROR,
      WRONG_RETURN_TYPE
    };
    ModuleInstance(
        WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> instance,
        std::shared_ptr<const CompartmentWrapper> compartment);

    ~ModuleInstance() override;

    outcome::result<PtrSize> callExportFunction(std::string_view name,
                                                PtrSize args) const override;

    outcome::result<boost::optional<WasmValue>> getGlobal(std::string_view name) const override;

   private:
    WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> instance_;
    std::shared_ptr<const CompartmentWrapper> compartment_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::wavm, ModuleInstance::Error)

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP
