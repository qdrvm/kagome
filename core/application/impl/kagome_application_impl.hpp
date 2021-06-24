/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_ALLINONEAPPLICATION
#define KAGOME_APPLICATION_ALLINONEAPPLICATION

#include "application/kagome_application.hpp"

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "injector/application_injector.hpp"

namespace kagome::application {

  class KagomeApplicationImpl : public KagomeApplication {
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~KagomeApplicationImpl() override = default;

    explicit KagomeApplicationImpl(const AppConfiguration &config);

    void run() override;

   private:
    const AppConfiguration &app_config_;
    uptr<injector::KagomeNodeInjector> injector_;
    log::Logger logger_;

    sptr<boost::asio::io_context> io_context_;
    sptr<AppStateManager> app_state_manager_;
    sptr<ChainSpec> chain_spec_;
    sptr<clock::SystemClock> clock_;
    sptr<consensus::babe::Babe> babe_;
    sptr<consensus::grandpa::Grandpa> grandpa_;
    sptr<metrics::Exposer> exposer_;
    sptr<network::Router> router_;
    sptr<network::PeerManager> peer_manager_;
    sptr<api::ApiService> jrpc_api_service_;
    sptr<network::SyncProtocolObserver> sync_observer_;
    const std::string node_name_;
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_ALLINONEAPPLICATION
