/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/module_repository_impl.hpp"

#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"

namespace kagome::runtime {

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      std::shared_ptr<const RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<const ModuleFactory> module_factory,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      const application::CodeSubstitutes &code_substitutes)
      : runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        module_factory_{std::move(module_factory)},
        header_repo_{std::move(header_repo)},
        code_substitutes_{code_substitutes} {
    BOOST_ASSERT(runtime_upgrade_tracker_);
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(header_repo_);
  }

  outcome::result<
      std::pair<std::shared_ptr<ModuleInstance>, InstanceEnvironment>>
  ModuleRepositoryImpl::getInstanceAt(
      std::shared_ptr<const RuntimeCodeProvider> code_provider,
      const primitives::BlockInfo &block) {
    OUTCOME_TRY(res, runtime_upgrade_tracker_->getRuntimeChangeBlock(block));

    OUTCOME_TRY(header, header_repo_->getBlockHeader(std::get<0>(res)));

    const auto &state = header.state_root;

    std::shared_ptr<Module> module;
    {
      std::lock_guard guard{modules_mutex_};
      if (auto it = modules_.find(state); it == modules_.end()) {
        auto code_it = code_substitutes_.find(block.hash);
        OUTCOME_TRY(code, code_provider->getCodeAt(state));
        OUTCOME_TRY(new_module,
                    module_factory_->make(code_substitutes_.end() != code_it
                                              ? code_it->second
                                              : code));
        module = std::move(new_module);
        if (std::get<1>(res)) {
          modules_[state] = module;
        }
      } else {
        module = it->second;
      }
    }

    {
      std::lock_guard guard{instances_mutex_};
      OUTCOME_TRY(instance_and_env, modules_[state]->instantiate());
      auto shared_instance = std::make_pair(
          std::shared_ptr<ModuleInstance>(std::move(instance_and_env.first)),
          std::move(instance_and_env.second));
      return shared_instance;
    }
  }

}  // namespace kagome::runtime
