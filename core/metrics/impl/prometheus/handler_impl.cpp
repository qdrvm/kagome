/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "handler_impl.hpp"
#include <prometheus/text_serializer.h>
#include "log/logger.hpp"
#include "registry_impl.hpp"

constexpr const char *EXPOSER_TRANSFERRED_BYTES_TOTAL =
    "exposer_transferred_bytes_total";
constexpr const char *EXPOSER_SCRAPES_TOTAL = "exposer_scrapes_total";
constexpr const char *EXPOSER_REQUEST_LATENCIES = "exposer_request_latencies";

using namespace prometheus;

std::vector<MetricFamily> CollectMetrics(
    const std::vector<std::weak_ptr<prometheus::Collectable>> &collectables) {
  auto collected_metrics = std::vector<MetricFamily>{};

  for (auto &&wcollectable : collectables) {
    auto collectable = wcollectable.lock();
    if (!collectable) {
      continue;
    }

    auto &&metrics = collectable->Collect();
    collected_metrics.insert(collected_metrics.end(),
                             std::make_move_iterator(metrics.begin()),
                             std::make_move_iterator(metrics.end()));
  }

  return collected_metrics;
}

namespace kagome::metrics {

  HandlerPtr createHandler() {
    return std::make_shared<PrometheusHandler>();
  }

  PrometheusHandler::PrometheusHandler()
      : logger_{log::createLogger("PrometheusHandler", "metrics")},
        registry_{createRegistry()} {
    registry_->registerCounterFamily(EXPOSER_TRANSFERRED_BYTES_TOTAL,
                                     "Transferred bytes to metrics services");
    bytes_transferred_ =
        registry_->registerCounterMetric(EXPOSER_TRANSFERRED_BYTES_TOTAL);
    registry_->registerCounterFamily(EXPOSER_SCRAPES_TOTAL,
                                     "Number of times metrics were scraped");
    num_scrapes_ = registry_->registerCounterMetric(EXPOSER_SCRAPES_TOTAL);
    registry_->registerSummaryFamily(
        EXPOSER_REQUEST_LATENCIES,
        "Latencies of serving scrape requests, in microseconds");
    request_latencies_ = registry_->registerSummaryMetric(
        EXPOSER_REQUEST_LATENCIES, {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});
  }

  void PrometheusHandler::onSessionRequest(Session::Request request,
                                           std::shared_ptr<Session> session) {
    auto start_time_of_request = std::chrono::steady_clock::now();

    std::vector<MetricFamily> metrics;

    {
      std::lock_guard<std::mutex> lock{collectables_mutex_};
      metrics = CollectMetrics(collectables_);
    }

    const TextSerializer serializer;

    auto size = writeResponse(session, request, serializer.Serialize(metrics));

    auto stop_time_of_request = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        stop_time_of_request - start_time_of_request);
    request_latencies_->observe(duration.count());

    bytes_transferred_->inc(size);
    num_scrapes_->inc();
  }

  std::size_t PrometheusHandler::writeResponse(std::shared_ptr<Session> session,
                                               Session::Request request,
                                               const std::string &body) {
    Session::Response res{boost::beast::http::status::ok, request.version()};
    res.set(boost::beast::http::field::content_type,
            "text/plain; charset=utf-8");
    res.set(boost::beast::http::field::content_length, body.size());
    res.keep_alive(request.keep_alive());
    res.body() = body;
    res.prepare_payload();
    session->respond(res);
    return body.size();
  }

  void PrometheusHandler::registerCollectable(Registry *registry) {
    registerCollectable(
        dynamic_cast<PrometheusRegistry *>(registry_.get())->registry());
  }

  void PrometheusHandler::registerCollectable(
      const std::weak_ptr<Collectable> &collectable) {
    std::lock_guard<std::mutex> lock{collectables_mutex_};
    cleanupStalePointers(collectables_);
    collectables_.push_back(collectable);
  }

  void PrometheusHandler::removeCollectable(
      const std::weak_ptr<Collectable> &collectable) {
    std::lock_guard<std::mutex> lock{collectables_mutex_};

    auto locked = collectable.lock();
    auto same_pointer = [&locked](const std::weak_ptr<Collectable> &candidate) {
      return locked == candidate.lock();
    };

    collectables_.erase(
        std::remove_if(
            std::begin(collectables_), std::end(collectables_), same_pointer),
        std::end(collectables_));
  }

  void PrometheusHandler::cleanupStalePointers(
      std::vector<std::weak_ptr<Collectable>> &collectables) {
    collectables.erase(
        std::remove_if(std::begin(collectables),
                       std::end(collectables),
                       [](const std::weak_ptr<Collectable> &candidate) {
                         return candidate.expired();
                       }),
        std::end(collectables));
  }

}  // namespace kagome::metrics
