/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP

#include <memory>

#include <boost/asio/io_context.hpp>

#include "clock/clock.hpp"
#include "storage/buffer_map_types.hpp"

namespace soralog {
  class LoggingSystem;
}

namespace kagome {
  namespace thread {
    struct ThreadPool;
  }

  namespace application {
    class AppConfiguration;
    class ChainSpec;
    class AppStateManager;
  }  // namespace application

  namespace application::mode {
    class PrintChainInfoMode;
    class RecoveryMode;
  }  // namespace application::mode

  namespace metrics {
    class Exposer;
    class MetricsWatcher;
  }  // namespace metrics

  namespace network {
    class Router;
    class PeerManager;
    class StateProtocolObserver;
    class SyncProtocolObserver;
  }  // namespace network

  namespace parachain {
    struct ParachainObserverImpl;
    struct ParachainProcessorImpl;
  }  // namespace parachain

  namespace runtime {
    class Executor;
  }

  namespace api {
    class ApiService;
  }

  namespace consensus::babe {
    class Babe;
  }

  namespace consensus::grandpa {
    class Grandpa;
  }

  namespace blockchain {
    class BlockStorage;
    class BlockTree;
  }  // namespace blockchain

  namespace storage::trie {
    class TrieStorage;
  }

  namespace telemetry {
    class TelemetryService;
  }

}  // namespace kagome

namespace kagome::injector {

  /**
   * Dependency injector for a universal node. Provides all major components
   * required by the kagome application.
   */
  class KagomeNodeInjector final {
   public:
    explicit KagomeNodeInjector(const application::AppConfiguration &);

    std::shared_ptr<application::ChainSpec> injectChainSpec();
    std::shared_ptr<blockchain::BlockStorage> injectBlockStorage();
    std::shared_ptr<application::AppStateManager> injectAppStateManager();
    std::shared_ptr<boost::asio::io_context> injectIoContext();
    std::shared_ptr<metrics::Exposer> injectOpenMetricsService();
    std::shared_ptr<network::Router> injectRouter();
    std::shared_ptr<network::PeerManager> injectPeerManager();
    std::shared_ptr<api::ApiService> injectRpcApiService();
    std::shared_ptr<clock::SystemClock> injectSystemClock();
    std::shared_ptr<consensus::babe::Babe> injectBabe();
    std::shared_ptr<network::StateProtocolObserver> injectStateObserver();
    std::shared_ptr<network::SyncProtocolObserver> injectSyncObserver();
    std::shared_ptr<parachain::ParachainObserverImpl> injectParachainObserver();
    std::shared_ptr<parachain::ParachainProcessorImpl>
    injectParachainProcessor();
    std::shared_ptr<thread::ThreadPool> injectThreadPool();
    std::shared_ptr<consensus::grandpa::Grandpa> injectGrandpa();
    std::shared_ptr<soralog::LoggingSystem> injectLoggingSystem();
    std::shared_ptr<storage::trie::TrieStorage> injectTrieStorage();
    std::shared_ptr<metrics::MetricsWatcher> injectMetricsWatcher();
    std::shared_ptr<telemetry::TelemetryService> injectTelemetryService();
    std::shared_ptr<blockchain::BlockTree> injectBlockTree();
    std::shared_ptr<runtime::Executor> injectExecutor();
    std::shared_ptr<storage::BufferStorage> injectStorage();

    std::shared_ptr<application::mode::PrintChainInfoMode>
    injectPrintChainInfoMode();
    std::shared_ptr<application::mode::RecoveryMode> injectRecoveryMode();

   protected:
    std::shared_ptr<class KagomeNodeInjectorImpl> pimpl_;
  };

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
