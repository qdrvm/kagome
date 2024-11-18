/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <boost/asio/io_context.hpp>

#include "clock/clock.hpp"
#include "network/dispute_request_observer.hpp"
#include "storage/spaced_storage.hpp"

namespace soralog {
  class LoggingSystem;
}

namespace kagome {
  namespace application {
    class AppConfiguration;
    class ChainSpec;
    class AppStateManager;
  }  // namespace application

  namespace application::mode {
    class PrintChainInfoMode;
    class PrecompileWasmMode;
    class RecoveryMode;
    class BenchmarkMode;
  }  // namespace application::mode

  namespace authority_discovery {
    class AddressPublisher;
  }  // namespace authority_discovery

  namespace benchmark {
    class BlockExecutionBenchmark;
  }

  namespace dispute {
    class DisputeCoordinator;
  }

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
    class ParachainObserver;
    class ParachainProcessorImpl;
    class ApprovalDistribution;

    namespace statement_distribution {
      class StatementDistribution;
    }
  }  // namespace parachain

  namespace runtime {
    class Executor;
  }

  namespace api {
    class ApiService;
  }

  namespace common {
    class MainThreadPool;
  }

  namespace consensus {
    class Timeline;
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

  class Watchdog;
}  // namespace kagome

namespace kagome::injector {

  /**
   * Dependency injector for a universal node. Provides all major components
   * required by the kagome application.
   */
  class KagomeNodeInjector final {
   public:
    explicit KagomeNodeInjector(
        std::shared_ptr<application::AppConfiguration> app_config);

    std::shared_ptr<application::AppConfiguration> injectAppConfig();
    std::shared_ptr<application::ChainSpec> injectChainSpec();
    std::shared_ptr<blockchain::BlockStorage> injectBlockStorage();
    std::shared_ptr<application::AppStateManager> injectAppStateManager();
    std::shared_ptr<boost::asio::io_context> injectIoContext();
    std::shared_ptr<Watchdog> injectWatchdog();
    std::shared_ptr<common::MainThreadPool> injectMainThreadPool();
    std::shared_ptr<metrics::Exposer> injectOpenMetricsService();
    std::shared_ptr<network::Router> injectRouter();
    std::shared_ptr<network::PeerManager> injectPeerManager();
    std::shared_ptr<api::ApiService> injectRpcApiService();
    std::shared_ptr<clock::SystemClock> injectSystemClock();
    std::shared_ptr<consensus::Timeline> injectTimeline();
    std::shared_ptr<network::SyncProtocolObserver> injectSyncObserver();
    std::shared_ptr<network::StateProtocolObserver> injectStateObserver();
    std::shared_ptr<parachain::ParachainObserver> injectParachainObserver();
    std::shared_ptr<parachain::ParachainProcessorImpl>
    injectParachainProcessor();
    std::shared_ptr<parachain::statement_distribution::StatementDistribution>
    injectStatementDistribution();
    std::shared_ptr<parachain::ApprovalDistribution>
    injectApprovalDistribution();
    std::shared_ptr<network::DisputeRequestObserver>
    injectDisputeRequestObserver();
    std::shared_ptr<dispute::DisputeCoordinator> injectDisputeCoordinator();
    std::shared_ptr<consensus::grandpa::Grandpa> injectGrandpa();
    std::shared_ptr<soralog::LoggingSystem> injectLoggingSystem();
    std::shared_ptr<storage::trie::TrieStorage> injectTrieStorage();
    std::shared_ptr<metrics::MetricsWatcher> injectMetricsWatcher();
    std::shared_ptr<telemetry::TelemetryService> injectTelemetryService();
    std::shared_ptr<blockchain::BlockTree> injectBlockTree();
    std::shared_ptr<runtime::Executor> injectExecutor();
    std::shared_ptr<storage::SpacedStorage> injectStorage();
    std::shared_ptr<authority_discovery::AddressPublisher>
    injectAddressPublisher();
    void kademliaRandomWalk();

    std::shared_ptr<application::mode::PrintChainInfoMode>
    injectPrintChainInfoMode();
    std::shared_ptr<application::mode::PrecompileWasmMode>
    injectPrecompileWasmMode();
    std::shared_ptr<application::mode::RecoveryMode> injectRecoveryMode();
    std::shared_ptr<benchmark::BlockExecutionBenchmark> injectBlockBenchmark();

   protected:
    std::shared_ptr<class KagomeNodeInjectorImpl> pimpl_;
  };

}  // namespace kagome::injector
