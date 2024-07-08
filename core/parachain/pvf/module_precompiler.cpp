/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/module_precompiler.hpp"

#include <atomic>

#include "parachain/pvf/pool.hpp"
#include "parachain/pvf/session_params.hpp"
#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {

  struct ParachainCore {
    runtime::CoreState state;
  };

  ModulePrecompiler::ModulePrecompiler(
      const kagome::parachain::ModulePrecompiler::Config &config,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<PvfPool> pvf_pool,
      std::shared_ptr<crypto::Hasher> hasher)
      : config_{config},
        parachain_api_{parachain_api},
        pvf_pool_{std::move(pvf_pool)},
        hasher_{hasher} {
    if (getThreadsNum() > std::thread::hardware_concurrency() - 1) {
      SL_WARN(
          log_,
          "The number of threads assigned for parachain runtime module "
          "pre-compilation is greater than (the number of hardware cores - 1). "
          "This is most likely inefficient.");
    }
  }

  struct ModulePrecompiler::PrecompilationStats {
    const size_t total_count{};
    std::atomic_int occupied_precompiled_count{};
    std::atomic_int scheduled_precompiled_count{};
    std::atomic_int total_code_size{};
  };

  std::optional<ParachainId> get_para_id(runtime::CoreState core) {
    return visit_in_place(
        core,
        [](const runtime::OccupiedCore &core) mutable
        -> std::optional<ParachainId> {
          return core.candidate_descriptor.para_id;
        },
        [](const runtime::ScheduledCore &core) mutable
        -> std::optional<ParachainId> { return core.para_id; },
        [](runtime::FreeCore) -> std::optional<ParachainId> {
          return std::nullopt;
        });
  }

  outcome::result<void> ModulePrecompiler::precompileModulesAt(
      const primitives::BlockHash &last_finalized) {
    OUTCOME_TRY(executor_params,
                sessionParams(*parachain_api_, last_finalized));
    auto cores_res = parachain_api_->availability_cores(last_finalized);
    if (cores_res.has_error()
        && cores_res.error()
               == runtime::RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND) {
      SL_WARN(log_,
              "Failed to warm up PVF executor runtime module cache, since "
              "ParachainHost API is not present in the runtime at block {}",
              last_finalized);
      return outcome::success();
    }
    OUTCOME_TRY(cores, cores_res);
    SL_DEBUG(log_,
             "Warming up PVF executor runtime instance cache at block {}",
             last_finalized);
    PrecompilationStats stats{
        .total_count = cores.size(),
    };
    auto start = std::chrono::steady_clock::now();

    std::mutex cores_queue_mutex;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < config_.precompile_threads_num; i++) {
      auto compilation_worker = [self = shared_from_this(),
                                 &executor_params,
                                 &cores_queue_mutex,
                                 &cores,
                                 &stats,
                                 &last_finalized,
                                 n = i + 1]() mutable -> outcome::result<void> {
        soralog::util::setThreadName(fmt::format("precompile.{}", n));
        while (true) {
          runtime::CoreState core;
          {
            std::scoped_lock l{cores_queue_mutex};
            if (cores.empty()) {
              break;
            }
            core = cores.back();
            cores.pop_back();
          }
          auto res = self->precompileModulesForCore(
              stats, last_finalized, executor_params, ParachainCore{core});
          if (!res) {
            using namespace std::string_literals;
            auto id = get_para_id(core);
            SL_ERROR(self->log_,
                     "Failed to precompile parachain module for {} parachain "
                     "core: {}",
                     id ? std::to_string(*id) : "empty"s,
                     res.error());
          }
        }
        return outcome::success();
      };
      threads.emplace_back(compilation_worker);
    }

    for (auto &t : threads) {
      t.join();
    }

    auto end = std::chrono::steady_clock::now();
    double time_taken =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count())
        / 1e3;
    SL_VERBOSE(log_,
               "Precompiled runtime instances for {} occupied parachain "
               "cores and {} scheduled parachain cores. Total code size is "
               "{}, time taken is {}s",
               stats.occupied_precompiled_count.load(),
               stats.scheduled_precompiled_count.load(),
               stats.total_code_size.load(),
               time_taken);
    return outcome::success();
  }

  outcome::result<void> ModulePrecompiler::precompileModulesForCore(
      PrecompilationStats &stats,
      const primitives::BlockHash &last_finalized,
      const runtime::RuntimeContext::ContextParams &executor_params,
      const ParachainCore &_core) {
    auto &core = _core.state;
    if (std::holds_alternative<runtime::FreeCore>(core)) {
      return outcome::success();

    } else if (std::holds_alternative<runtime::OccupiedCore>(core)) {
      SL_TRACE(log_, "Precompile for occupied availability core");
      stats.occupied_precompiled_count++;
    } else if (std::holds_alternative<runtime::ScheduledCore>(core)) {
      SL_TRACE(log_, "Precompile for scheduled availability core");
      stats.scheduled_precompiled_count++;
    }
    // since we eliminated empty core option earlier
    auto para_id = get_para_id(core).value();
    OUTCOME_TRY(code_opt,
                parachain_api_->validation_code(
                    last_finalized,
                    para_id,
                    runtime::OccupiedCoreAssumption::Included));
    if (!code_opt) {
      SL_WARN(log_,
              "No validation code found for parachain {} with 'included' "
              "occupied assumption",
              para_id);
      return outcome::success();
    }
    auto &code = *code_opt;
    auto hash = hasher_->blake2b_256(code);
    SL_DEBUG(log_,
             "Validation code for parachain {} has size {} and hash {}",
             para_id,
             code.size(),
             hash);
    stats.total_code_size += code.size();

    OUTCOME_TRY(pvf_pool_->precompile(hash, code, executor_params));
    SL_DEBUG(log_,
             "Instantiated runtime instance with code hash {} for parachain "
             "{}, {} left",
             hash,
             para_id,
             stats.total_count - stats.occupied_precompiled_count
                 - stats.scheduled_precompiled_count);

    return outcome::success();
  }

}  // namespace kagome::parachain
