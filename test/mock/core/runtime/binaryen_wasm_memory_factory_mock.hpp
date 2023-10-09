/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/binaryen/binaryen_memory_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::binaryen {

  class BinaryenWasmMemoryFactoryMock : public BinaryenMemoryFactory {
   public:
    ~BinaryenWasmMemoryFactoryMock() override = default;

    MOCK_METHOD(std::unique_ptr<MemoryImpl>,
                make,
                (RuntimeExternalInterface::InternalMemory * memory,
                 const MemoryConfig &config),
                (const, override));
  };

}  // namespace kagome::runtime::binaryen
