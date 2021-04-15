/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_VALIDATING_NODE_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_IMPL_VALIDATING_NODE_APPLICATION_HPP

#include "application/kagome_application.hpp"

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "injector/application_injector.hpp"

namespace kagome::application {

  class AllInOneApplication : public KagomeApplication {
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~AllInOneApplication() override = default;

    explicit AllInOneApplication(const AppConfiguration &config);

    void run() override;

   private:
    const AppConfiguration &app_config_;
    uptr<injector::ValidatingNodeInjector> injector_;
    log::Logger logger_;

    sptr<boost::asio::io_context> io_context_;
    sptr<AppStateManager> app_state_manager_;
    sptr<ChainSpec> chain_spec_;
    sptr<clock::SystemClock> clock_;
    sptr<consensus::babe::Babe> babe_;
    sptr<consensus::grandpa::Grandpa> grandpa_;
    sptr<network::Router> router_;
    sptr<network::PeerManager> peer_manager_;
    sptr<api::ApiService> jrpc_api_service_;
    const std::string node_name_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_VALIDATING_NODE_APPLICATION_HPP
