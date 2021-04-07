/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP

#include <memory>

#include <boost/asio/io_context.hpp>

#include "clock/clock.hpp"

namespace soralog {
  class LoggingSystem;
}

namespace kagome {
  namespace application {
    class AppConfiguration;
    class ChainSpec;
    class AppStateManager;
  }  // namespace application

  namespace network {
    class Router;
    class PeerManager;
  }  // namespace network

  namespace api {
    class ApiService;
  }

  namespace consensus::babe {
    class Babe;
  }

  namespace consensus::grandpa {
    class Grandpa;
  }
}  // namespace kagome

namespace kagome::injector {

  /**
   * Dependency injector for a syncing node. Provides all major components
   * required by the kagome syncing node application.
   */
  class SyncingNodeInjector final {
   public:
    explicit SyncingNodeInjector(const application::AppConfiguration &);

    std::shared_ptr<application::ChainSpec> injectChainSpec();
    std::shared_ptr<application::AppStateManager> injectAppStateManager();
    std::shared_ptr<boost::asio::io_context> injectIoContext();
    std::shared_ptr<network::Router> injectRouter();
    std::shared_ptr<network::PeerManager> injectPeerManager();
    std::shared_ptr<api::ApiService> injectRpcApiService();

   protected:
    std::shared_ptr<class SyncingNodeInjectorImpl> pimpl_;
  };

  /**
   * Dependency injector for a validating node. Provides all major components
   * required by the kagome validating node application.
   */
  class ValidatingNodeInjector final {
   public:
    explicit ValidatingNodeInjector(const application::AppConfiguration &);

    std::shared_ptr<application::ChainSpec> injectChainSpec();
    std::shared_ptr<application::AppStateManager> injectAppStateManager();
    std::shared_ptr<boost::asio::io_context> injectIoContext();
    std::shared_ptr<network::Router> injectRouter();
    std::shared_ptr<network::PeerManager> injectPeerManager();
    std::shared_ptr<api::ApiService> injectRpcApiService();
    std::shared_ptr<clock::SystemClock> injectSystemClock();
    std::shared_ptr<consensus::babe::Babe> injectBabe();
    std::shared_ptr<consensus::grandpa::Grandpa> injectGrandpa();
    std::shared_ptr<soralog::LoggingSystem> injectLoggingSystem();

   protected:
    std::shared_ptr<class ValidatingNodeInjectorImpl> pimpl_;
  };

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
