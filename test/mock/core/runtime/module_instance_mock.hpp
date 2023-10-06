/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_instance.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class ModuleInstanceMock : public ModuleInstance {
   public:
    MOCK_METHOD(const common::Hash256 &, getCodeHash, (), (const, override));

    MOCK_METHOD(std::shared_ptr<const Module>,
                getModule,
                (),
                (const, override));

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

    MOCK_METHOD(const InstanceEnvironment &,
                getEnvironment,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<void>, resetEnvironment, (), (override));

    MOCK_METHOD(outcome::result<void>,
                resetMemory,
                (const MemoryLimits &limits),
                (override));
  };
}  // namespace kagome::runtime
