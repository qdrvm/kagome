/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <cstdint>

#include "runtime/common/memory_allocator.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"
#include "runtime/wavm/memory_impl.hpp"
#include "runtime/wavm/module_params.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::literals::operator""_MB;
using kagome::runtime::kDefaultHeapBase;
using kagome::runtime::kInitialMemorySize;
using kagome::runtime::MemoryAllocator;
using kagome::runtime::MemoryConfig;
using kagome::runtime::roundUpAlign;
using kagome::runtime::wavm::CompartmentWrapper;
using kagome::runtime::wavm::IntrinsicModule;
using kagome::runtime::wavm::IntrinsicModuleInstance;
using kagome::runtime::wavm::MemoryImpl;
using kagome::runtime::wavm::ModuleParams;

class WavmMemoryHeapTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    using std::string_literals::operator""s;

    auto compartment_wrapper =
        std::make_shared<CompartmentWrapper>("WAVM Memory Test compartment"s);
    auto module_params = std::make_shared<ModuleParams>();
    auto intr_module = std::make_shared<kagome::runtime::wavm::IntrinsicModule>(
        compartment_wrapper, module_params->intrinsicMemoryType);
    // need this just because there is an assert in intrinsic module which
    // prevents it from being instantiated with zero functions
    intr_module->addFunction(
        "stub",
        static_cast<void (*)(WAVM::Runtime::ContextRuntimeData *)>(
            [](WAVM::Runtime::ContextRuntimeData *) {}),
        WAVM::IR::FunctionType{});
    instance_ = intr_module->instantiate();

    memory_ = std::make_unique<MemoryImpl>(instance_->getExportedMemory(),
                                           MemoryConfig{kDefaultHeapBase, {}});
  }

  static const uint32_t memory_size_ = kInitialMemorySize;
  static const uint32_t memory_page_limit_ = 512_MB;

  std::unique_ptr<MemoryImpl> memory_;
  std::unique_ptr<IntrinsicModuleInstance> instance_;
};

/**
 * @given arbitrary buffer of size N
 * @when this buffer is stored in memory heap @and then load of N bytes is done
 * @then the same buffer is returned
 */
TEST_F(WavmMemoryHeapTest, LoadNTest) {
  const size_t N = 3;

  kagome::common::Buffer b(N, 'c');

  auto ptr = memory_->allocate(N);

  memory_->storeBuffer(ptr, b);

  auto res_b = memory_->loadN(ptr, N);
  ASSERT_EQ(b, res_b);
}
