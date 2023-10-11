/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/module_precompiler.hpp"

#include <atomic>
#include <future>
#include <ranges>

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
      std::shared_ptr<runtime::RuntimeInstancesPool> runtime_cache,
      std::shared_ptr<crypto::Hasher> hasher)
      : config_{config},
        parachain_api_{parachain_api},
        runtime_cache_{runtime_cache},
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

  outcome::result<void> ModulePrecompiler::precompileModulesAt(
      const primitives::BlockHash &last_finalized) {
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
    OUTCOME_TRY(cores_res);
    auto &cores = cores_res.value();
    SL_DEBUG(log_,
             "Warming up PVF executor runtime instance cache at block {}",
             last_finalized);
    PrecompilationStats stats{
        .total_count = cores.size(),
    };
    auto start = std::chrono::steady_clock::now();

    std::mutex cores_queue_mutex;
    std::vector<std::future<outcome::result<void>>> futures;
    for (size_t i = 0; i < config_.precompile_threads_num; i++) {
      auto compilation_worker =
          [self = shared_from_this(),
           &cores_queue_mutex,
           &cores,
           &stats,
           &last_finalized]() mutable -> outcome::result<void> {
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
          OUTCOME_TRY(self->precompileModulesForCore(
              stats, last_finalized, ParachainCore{core}));
        }
        return outcome::success();
      };
      futures.emplace_back(std::async(std::launch::async, compilation_worker));
    }

    for (auto &f : futures) {
      auto res = f.get();
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
      const ParachainCore &_core) {
    auto &core = _core.state;
    // empty core
    if (std::holds_alternative<runtime::FreeCore>(core)) {
      return outcome::success();
    }
    auto para_id = visit_in_place(
        core,
        [this, &stats](const runtime::OccupiedCore &core) mutable {
          SL_TRACE(log_, "Precompile for occupied availability core");
          stats.occupied_precompiled_count++;
          return core.candidate_descriptor.para_id;
        },
        [this, &stats](const runtime::ScheduledCore &core) mutable {
          SL_TRACE(log_, "Precompile for scheduled availability core");
          stats.scheduled_precompiled_count++;
          return core.para_id;
        },
        [](runtime::FreeCore) -> ParachainId { BOOST_UNREACHABLE_RETURN({}) });
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
    OUTCOME_TRY(runtime_cache_->instantiateFromCode(hash, code));
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
