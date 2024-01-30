/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <libp2p/log/configurator.hpp>

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/common/asio_buffer.hpp>

#include "common/bytestr.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"
#include "parachain/pvf/kagome_pvf_worker_injector.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "scale/scale.hpp"

#include "parachain/pvf/kagome_pvf_worker.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"

// rust reference: polkadot-sdk/polkadot/node/core/pvf/execute-worker/src/lib.rs

namespace kagome::parachain {
  static kagome::log::Logger logger;

  outcome::result<void> readStdin(std::span<uint8_t> out) {
    std::cin.read(reinterpret_cast<char *>(out.data()), out.size());
    if (not std::cin.good()) {
      return std::errc::io_error;
    }
    return outcome::success();
  }

  auto dummyRuntimeContext(std::shared_ptr<runtime::ModuleInstance> instance) {
    return runtime::RuntimeContext::create_TEST(instance);
  }

  outcome::result<PvfWorkerInput> decodeInput() {
    std::array<uint8_t, sizeof(uint32_t)> length_bytes;
    OUTCOME_TRY(readStdin(length_bytes));
    OUTCOME_TRY(message_length, scale::decode<uint32_t>(length_bytes));
    std::vector<uint8_t> packed_message(message_length, 0);
    OUTCOME_TRY(readStdin(packed_message));
    OUTCOME_TRY(pvf_worker_input,
                scale::decode<PvfWorkerInput>(packed_message));
    return pvf_worker_input;
  }

  outcome::result<std::shared_ptr<runtime::ModuleFactory>> createModuleFactory(
      const auto &injector, RuntimeEngine engine) {
    switch (engine) {
      case RuntimeEngine::kBinaryen:
        return injector.template create<
            std::shared_ptr<runtime::binaryen::ModuleFactoryImpl>>();
      case RuntimeEngine::kWAVM:
        // About ifdefs - looks bad, but works as it ought to
#if KAGOME_WASM_COMPILER_WAVM == 1
        return injector.template create<
            std::shared_ptr<runtime::wavm::ModuleFactoryImpl>>();
#else
        SL_ERROR(logger, "WAVM runtime engine is not supported");
        return std::errc::not_supported;
#endif
      case RuntimeEngine::kWasmEdgeInterpreted:
      case RuntimeEngine::kWasmEdgeCompiled:
#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
        return injector.template create<
            std::shared_ptr<runtime::wasm_edge::ModuleFactoryImpl>>();
#else
        SL_ERROR(logger, "WasmEdge runtime engine is not supported");
        return std::errc::not_supported;
#endif
      default:
        SL_ERROR(logger, "Unknown runtime engine is requested");
        return std::errc::not_supported;
    }
  }

  outcome::result<void> pvf_worker_main_outcome() {
    OUTCOME_TRY(input, decodeInput());
    kagome::log::tuneLoggingSystem(input.log_params);
    auto injector = pvf_worker_injector(input);
    OUTCOME_TRY(factory, createModuleFactory(injector, input.engine));
    OUTCOME_TRY(module, factory->make(input.runtime_code));
    OUTCOME_TRY(instance, module->instantiate());
    OUTCOME_TRY(instance->resetMemory({}));
    auto dummy_context = dummyRuntimeContext(instance);
    OUTCOME_TRY(result,
                instance->callExportFunction(
                    dummy_context, input.function, input.params));
    OUTCOME_TRY(instance->resetEnvironment());
    OUTCOME_TRY(len, scale::encode<uint32_t>(result.size()));
    std::cout.write((const char *)len.data(), len.size());
    std::cout.write((const char *)result.data(), result.size());
    return outcome::success();
  }

  int pvf_worker_main(int argc, const char **argv) {
    auto logging_system = std::make_shared<soralog::LoggingSystem>(
        std::make_shared<kagome::log::Configurator>(
            std::make_shared<libp2p::log::Configurator>()));
    auto r = logging_system->configure();
    if (not r.message.empty()) {
      (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
    }
    if (r.has_error) {
      exit(EXIT_FAILURE);
    }
    kagome::log::setLoggingSystem(logging_system);
    logger = kagome::log::createLogger("Pvf Worker", "parachain");

    if (auto r = pvf_worker_main_outcome(); not r) {
      SL_ERROR(logger, "{}", r.error());
      return EXIT_FAILURE;
    }
    return 0;
  }
}  // namespace kagome::parachain
