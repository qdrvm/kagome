/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/kagome_application.hpp"

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"

namespace kagome::injector {
  class KagomeNodeInjector;
}

namespace kagome::application {
  class Mode;

  class KagomeApplicationImpl final : public KagomeApplication {
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~KagomeApplicationImpl() override;

    explicit KagomeApplicationImpl(injector::KagomeNodeInjector &injector);

    int chainInfo() override;

    int precompileWasm() override;

    int recovery() override;

    void run() override;

   private:
    int runMode(Mode &mode);

    injector::KagomeNodeInjector &injector_;

    std::shared_ptr<AppConfiguration> app_config_;
    std::shared_ptr<ChainSpec> chain_spec_;

    log::Logger logger_;
  };

}  // namespace kagome::application
