/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_ALLINONEAPPLICATION
#define KAGOME_APPLICATION_ALLINONEAPPLICATION

#include "application/kagome_application.hpp"

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "injector/application_injector.hpp"

namespace kagome::application {

  class KagomeApplicationImpl : public KagomeApplication {
   public:
    ~KagomeApplicationImpl() override = default;

    explicit KagomeApplicationImpl(std::shared_ptr<AppConfiguration> config);

    int chainInfo() override;

    int recovery() override;

    void run() override;

   private:
    std::shared_ptr<AppConfiguration> app_config_;

    std::unique_ptr<injector::KagomeNodeInjector> injector_;
    std::shared_ptr<ChainSpec> chain_spec_;

    log::Logger logger_;
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_ALLINONEAPPLICATION
