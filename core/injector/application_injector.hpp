/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP

#include <memory>

#include <boost/asio/io_context.hpp>

#include "clock/clock.hpp"

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

  class SyncingNodeInjector {
   public:
    explicit SyncingNodeInjector(const application::AppConfiguration&);

    std::shared_ptr<application::ChainSpec> injectChainSpec() noexcept;
    std::shared_ptr<application::AppStateManager>
    injectAppStateManager() noexcept;
    std::shared_ptr<boost::asio::io_context> injectIoContext() noexcept;
    std::shared_ptr<network::Router> injectRouter() noexcept;
    std::shared_ptr<network::PeerManager> injectPeerManager() noexcept;
    std::shared_ptr<api::ApiService> injectRpcApiService() noexcept;

   protected:
    std::shared_ptr<class SyncingNodeInjectorImpl> pimpl_;
  };

  class ValidatingNodeInjector {
   public:
    explicit ValidatingNodeInjector(const application::AppConfiguration&);

    std::shared_ptr<application::ChainSpec> injectChainSpec() noexcept;
    std::shared_ptr<application::AppStateManager>
    injectAppStateManager() noexcept;
    std::shared_ptr<boost::asio::io_context> injectIoContext() noexcept;
    std::shared_ptr<network::Router> injectRouter() noexcept;
    std::shared_ptr<network::PeerManager> injectPeerManager() noexcept;
    std::shared_ptr<api::ApiService> injectRpcApiService() noexcept;
    std::shared_ptr<clock::SystemClock> injectSystemClock() noexcept;
    std::shared_ptr<consensus::babe::Babe> injectBabe() noexcept;
    std::shared_ptr<consensus::grandpa::Grandpa> injectGrandpa() noexcept;

   protected:
    std::shared_ptr<class ValidatingNodeInjectorImpl> pimpl_;
  };


}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
