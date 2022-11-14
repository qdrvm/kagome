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
    MOCK_METHOD(const common::Hash256 &, getCodeHash, (), (const, override));

    MOCK_METHOD(outcome::result<PtrSize>,
                callExportFunction,
                (std::string_view name, common::BufferView args),
                (const, override));

    MOCK_METHOD(void,
                forDataSegment,
                (const DataSegmentProcessor &),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<WasmValue>>,
                getGlobal,
                (std::string_view name),
                (const, override));

    MOCK_METHOD(InstanceEnvironment const &,
                getEnvironment,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<void>, resetEnvironment, (), (override));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_INSTANCE_MOCK_HPP
