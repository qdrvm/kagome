/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/kagome_application_impl.hpp"

#include <thread>

#include "application/app_state_manager.hpp"
#include "application/impl/util.hpp"
#include "application/modes/print_chain_info_mode.hpp"
#include "application/modes/recovery_mode.hpp"
#include "metrics/metrics.hpp"
#include "telemetry/service.hpp"

namespace kagome::application {
  KagomeApplicationImpl::~KagomeApplicationImpl() {
    kagome::telemetry::setTelemetryService(nullptr);
  }

  KagomeApplicationImpl::KagomeApplicationImpl(
      std::shared_ptr<AppConfiguration> app_config)
      : app_config_(app_config),
        injector_{std::make_unique<injector::KagomeNodeInjector>(app_config)},
        logger_(log::createLogger("Application", "application")) {
    // keep important instances, they must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    chain_spec_ = injector_->injectChainSpec();
    BOOST_ASSERT(chain_spec_ != nullptr);
  }

  int KagomeApplicationImpl::chainInfo() {
    auto mode = injector_->injectPrintChainInfoMode();
    return mode->run();
  }

  int KagomeApplicationImpl::recovery() {
    logger_->info("Start in recovery mode with PID {}", getpid());

    auto mode = injector_->injectRecoveryMode();
    return mode->run();
  }

  void KagomeApplicationImpl::run() {
    auto app_state_manager = injector_->injectAppStateManager();
    auto io_context = injector_->injectIoContext();
    auto clock = injector_->injectSystemClock();

    injector_->injectOpenMetricsService();
    injector_->injectRpcApiService();

    kagome::telemetry::setTelemetryService(injector_->injectTelemetryService());

    injector_->injectAddressPublisher();

    logger_->info("Start as node version '{}' named as '{}' with PID {}",
                  app_config_->nodeVersion(),
                  app_config_->nodeName(),
                  getpid());

    auto chain_path = app_config_->chainPath(chain_spec_->id());
    auto storage_backend = app_config_->storageBackend()
                                == AppConfiguration::StorageBackend::RocksDB
                             ? "RocksDB"
                             : "Unknown";
    logger_->info(
        "Chain path is {}, storage backend is {}", chain_path, storage_backend);
    auto res = util::init_directory(chain_path);
    if (not res) {
      logger_->critical("Error initializing chain directory {}: {}",
                        chain_path.native(),
                        res.error());
      exit(EXIT_FAILURE);
    }

    app_state_manager->atLaunch([ctx{io_context}, log{logger_}] {
      std::thread asio_runner([ctx{ctx}, log{log}] { ctx->run(); });
      asio_runner.detach();
      return true;
    });

    app_state_manager->atShutdown([ctx{io_context}] { ctx->stop(); });

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

    app_state_manager->run();
  }

}  // namespace kagome::application
