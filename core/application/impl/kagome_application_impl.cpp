/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/kagome_application_impl.hpp"

#include <cstdlib>
#include <soralog/macro.hpp>
#include <thread>

#include "application/app_state_manager.hpp"
#include "application/impl/util.hpp"
#include "application/modes/precompile_wasm.hpp"
#include "application/modes/print_chain_info_mode.hpp"
#include "application/modes/recovery_mode.hpp"
#include "injector/application_injector.hpp"
#include "metrics/metrics.hpp"
#include "parachain/pvf/secure_mode_precheck.hpp"
#include "storage/migrations/migrations.hpp"
#include "telemetry/service.hpp"
#include "utils/watchdog.hpp"

namespace kagome::application {
  KagomeApplicationImpl::~KagomeApplicationImpl() {
    kagome::telemetry::setTelemetryService(nullptr);
  }

  KagomeApplicationImpl::KagomeApplicationImpl(
      injector::KagomeNodeInjector &injector)
      : injector_(injector),
        logger_(log::createLogger("Application", "application")) {
    // keep important instances, they must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    app_config_ = injector_.injectAppConfig();
    BOOST_ASSERT(app_config_ != nullptr);
    chain_spec_ = injector_.injectChainSpec();
    BOOST_ASSERT(chain_spec_ != nullptr);
  }

  int KagomeApplicationImpl::chainInfo() {
    return runMode(*injector_.injectPrintChainInfoMode());
  }

  int KagomeApplicationImpl::precompileWasm() {
    return runMode(*injector_.injectPrecompileWasmMode());
  }

  int KagomeApplicationImpl::recovery() {
    logger_->info("Start in recovery mode with PID {}", getpid());
    return runMode(*injector_.injectRecoveryMode());
  }

  void KagomeApplicationImpl::run() {
    auto app_state_manager = injector_.injectAppStateManager();
    auto clock = injector_.injectSystemClock();
    auto watchdog = injector_.injectWatchdog();

    injector_.injectOpenMetricsService();
    injector_.injectRpcApiService();

    kagome::telemetry::setTelemetryService(injector_.injectTelemetryService());

    injector_.kademliaRandomWalk();
    injector_.injectAddressPublisher();
    injector_.injectTimeline();

    logger_->info("Start as node version '{}' named as '{}' with PID {}",
                  app_config_->nodeVersion(),
                  app_config_->nodeName(),
                  getpid());

    auto chain_path = app_config_->chainPath(chain_spec_->id());
    const char *storage_backend =
        app_config_->storageBackend()
                == AppConfiguration::StorageBackend::RocksDB
            ? "RocksDB"
            : "Unknown";
    logger_->info("Chain path is {}, storage backend is {}",
                  chain_path.native(),
                  storage_backend);
    auto res = util::init_directory(chain_path);
    if (not res) {
      logger_->critical("Error initializing chain directory {}: {}",
                        chain_path.native(),
                        res.error());
      exit(EXIT_FAILURE);
    }

    std::thread watchdog_thread([watchdog] {
      soralog::util::setThreadName("watchdog");
      watchdog->checkLoop(kWatchdogDefaultTimeout);
    });

    app_state_manager->atShutdown([watchdog] { watchdog->stop(); });

    {  // Metrics
      auto metrics_registry = metrics::createRegistry();

      constexpr auto startTimeMetricName = "kagome_process_start_time_seconds";
      metrics_registry->registerGaugeFamily(
          startTimeMetricName,
          "UNIX timestamp of the moment the process started");
      auto metric_start_time =
          metrics_registry->registerGaugeMetric(startTimeMetricName);
      metric_start_time->set(clock->nowUint64());

      constexpr auto nodeRolesMetricName = "kagome_node_roles";
      metrics_registry->registerGaugeFamily(nodeRolesMetricName,
                                            "The roles the node is running as");
      auto metric_node_roles =
          metrics_registry->registerGaugeMetric(nodeRolesMetricName);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
      metric_node_roles->set(app_config_->roles().value);

      constexpr auto buildInfoMetricName = "kagome_build_info";
      metrics_registry->registerGaugeFamily(
          buildInfoMetricName,
          "A metric with a constant '1' value labeled by name, version");
      auto metric_build_info = metrics_registry->registerGaugeMetric(
          buildInfoMetricName,
          {{"name", app_config_->nodeName()},
           {"version", app_config_->nodeVersion()}});
      metric_build_info->set(1);
    }

#ifdef __linux__
    if (not app_config_->disableSecureMode() and app_config_->usePvfSubprocess()
        and app_config_->roles().isAuthority()) {
      auto res = parachain::runSecureModeCheckProcess(
          *injector_.injectIoContext(), app_config_->runtimeCacheDirPath());
      if (!res) {
        SL_ERROR(logger_, "Secure mode check failed: {}", res.error());
        exit(EXIT_FAILURE);
      }
      if (!res.assume_value().isTotallySupported()) {
        SL_ERROR(logger_,
                 "Secure mode is not supported completely. You can disable it "
                 "using --insecure-validator-i-know-what-i-do.");
        exit(EXIT_FAILURE);
      }
    }
#else
    SL_WARN(logger_,
            "Secure validator mode is not implemented for the current "
            "platform. Proceed at your own risk.");
#endif

    if (app_config_->enableDbMigration()) {
      if (auto res = storage::migrations::runMigrations(injector_); !res) {
        SL_ERROR(logger_, "Failed to migrate the database: {}", res.error());
        exit(EXIT_FAILURE);
      }
    }

    app_state_manager->run();

    watchdog->stop();

    watchdog_thread.join();
  }

  int KagomeApplicationImpl::runMode(Mode &mode) {
    auto watchdog = injector_.injectWatchdog();
    auto r = mode.run();
    watchdog->stop();
    return r;
  }
}  // namespace kagome::application
