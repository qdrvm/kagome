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
#include "parachain/pvf/run_worker.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"

#include "test_wasm.hpp"

// rust reference: polkadot-sdk/polkadot/node/core/pvf/execute-worker/src/lib.rs

namespace kagome::parachain {
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
    fmt::println(stderr, "DEBUG: decodeInput: read 4");
    OUTCOME_TRY(readStdin(length_bytes));
    fmt::println(stderr, "DEBUG: decodeInput: read 4: ok");
    OUTCOME_TRY(message_length, scale::decode<uint32_t>(length_bytes));
    fmt::println(stderr, "DEBUG: decodeInput: read 4: {}", message_length);
    std::vector<uint8_t> packed_message(message_length, 0);
    OUTCOME_TRY(readStdin(packed_message));
    fmt::println(stderr, "DEBUG: decodeInput: read {}: ok", message_length);
    fmt::println(stderr,
                 "DEBUG:   dump {}",
                 common::hex_lower(libp2p::BytesIn{packed_message}.first(40)));
    OUTCOME_TRY(pvf_worker_input,
                scale::decode<PvfWorkerInput>(packed_message));
    return pvf_worker_input;
  }

  outcome::result<std::shared_ptr<runtime::ModuleFactory>> createModuleFactory(
      const auto &injector, RuntimeEngine engine) {
    switch (engine) {
      case RuntimeEngine::kBinaryen:
        // return injector.template create
        //     std::shared_ptr<runtime::binaryen::ModuleFactoryImpl>>();
      case RuntimeEngine::kWAVM:
#if KAGOME_WASM_COMPILER_WAVM == 1
        return injector.template create<
            std::shared_ptr<runtime::wavm::ModuleFactoryImpl>>();
#else
        fmt::println(stderr, "WAVM runtime engine is not supported");
        return std::errc::not_supported;
#endif
      case RuntimeEngine::kWasmEdge:
#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
        return injector.template create<
            std::shared_ptr<runtime::wasm_edge::ModuleFactoryImpl>>();
#else
        fmt::println(stderr, "WasmEdge runtime engine is not supported");
        return std::errc::not_supported;
#endif
      default:
        return std::errc::not_supported;
    }
  }

  PvfWorkerInput test_input{
      RuntimeEngine::kWAVM,
      kTestWasm,
      "validate_block",
      kTestWasmArgs,
      std::filesystem::temp_directory_path() / "kagome/runtimes-cache",
  };

  outcome::result<void> pvf_worker_main_outcome() {
    OUTCOME_TRY(input, decodeInput());
    auto injector = pvf_worker_injector(input);
    OUTCOME_TRY(factory, createModuleFactory(injector, input.engine));
    // maybe uncompress
    OUTCOME_TRY(module, factory->make(input.runtime_code));
    OUTCOME_TRY(instance, module->instantiate());
    OUTCOME_TRY(instance->resetMemory({}));
    auto dummy_context = dummyRuntimeContext(instance);
    OUTCOME_TRY(result,
                instance->callExportFunction(
                    dummy_context, input.function, input.params));
    OUTCOME_TRY(instance->resetEnvironment());
    fmt::println(stderr, "debug: result: {}", common::hex_lower(result));
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

    if (true && argc <= 1) {
      auto io_context = std::make_shared<boost::asio::io_context>();
      auto scheduler = std::make_shared<libp2p::basic::SchedulerImpl>(
          std::make_shared<libp2p::basic::AsioSchedulerBackend>(io_context),
          libp2p::basic::Scheduler::Config{});
      constexpr auto kChildTimeout = std::chrono::seconds(6);

      common::Buffer input{scale::encode(test_input).value()};
      fmt::println(stderr,
                   "DEBUG:   send {}",
                   common::hex_lower(libp2p::BytesIn{input}.first(40)));
      runWorker(*io_context,
                scheduler,
                kChildTimeout,
                argv[0],
                input,
                [&](outcome::result<common::Buffer> result) {
                  fmt::println("parent: result: {}",
                               common::hex_lower(result.value()));
                  io_context->stop();
                });

      io_context->run();
      fmt::println("main: end");

      // namespace bp = boost::process;

      // auto io_context = std::make_shared<boost::asio::io_context>();
      // bp::async_pipe child_out_async_pipe(*io_context);

      // auto input = scale::encode(test_input).value();
      // auto len = scale::encode<uint32_t>(input.size()).value();

      // bp::ipstream child_out, child_err;  // for reading program output
      // bp::opstream child_in;              // for passing input to a child

      // // bp::search_path
      // bp::child c(argv[0],
      //             bp::args({"--do-work"}),
      //             bp::std_out > child_out_async_pipe,  // child_out,
      //             bp::std_in<child_in, bp::std_err> child_err);

      // constexpr auto kChildTimeout = std::chrono::seconds(6);
      // auto scheduler = std::make_shared<libp2p::basic::SchedulerImpl>(
      //     std::make_shared<libp2p::basic::AsioSchedulerBackend>(io_context),
      //     libp2p::basic::Scheduler::Config{});
      // auto deadline_timer_handle = scheduler->scheduleWithHandle(
      //     [&]() {
      //       c.terminate();
      //       fmt::println("Deadline reached\n");
      //     },
      //     kChildTimeout);

      // // if (!c.wait_for(kChildTimeout)) {
      // //   c.terminate();
      // //   fmt::println()
      // // }

      // child_in.write((const char *)len.data(), len.size());
      // child_in.write((const char *)input.data(), input.size());
      // child_in.close();

      // // comm
      // std::vector<uint8_t> child_out_buffer(10 * 1024);
      // boost::asio::async_read(
      //     child_out_async_pipe,
      //     libp2p::asioBuffer(libp2p::BytesOut{child_out_buffer}),
      //     [&](const boost::system::error_code &ec, std::size_t size) {
      //       fmt::println("worker: [size = {}, {}]",
      //                    size,
      //                    byte2str(child_out_buffer).substr(0, size));
      //       deadline_timer_handle.cancel();
      //       deadline_timer_handle.cancel();
      //       io_context->stop();
      //     });

      // // auto wg = boost::asio::make_work_guard(io_context);
      // io_context->run();

      // // std::string tmp_output;
      // // child_out >> tmp_output;
      // // fmt::println("worker: [{}]", byte2str(child_out_buffer));
      // // child_out >> tmp_output;
      // c.wait();
      return 0;
    }

    // if (true) {
    //   fmt::println(stderr, "SLEEP");
    //   sleep(5);
    // };
    if (auto r = pvf_worker_main_outcome(); not r) {
      fmt::println(stderr, "{}", r.error());
      return EXIT_FAILURE;
    }
    return 0;
  }
}  // namespace kagome::parachain

int main(int argc, const char **argv) {
  return kagome::parachain::pvf_worker_main(argc, argv);
}
