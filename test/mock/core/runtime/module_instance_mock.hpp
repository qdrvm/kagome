/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_INSTANCE_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_INSTANCE_MOCK_HPP

#include "runtime/module_instance.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class ModuleInstanceMock : public ModuleInstance {
   public:
    MOCK_CONST_METHOD2(callExportFunction,
                       outcome::result<PtrSize>(std::string_view name,
                                                PtrSize args));

    MOCK_CONST_METHOD1(
        getGlobal,
        outcome::result<boost::optional<WasmValue>>(std::string_view name));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_INSTANCE_MOCK_HPP
