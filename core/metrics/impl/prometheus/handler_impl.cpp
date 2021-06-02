/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "metrics/impl/prometheus/handler_impl.hpp"

#include <prometheus/text_serializer.h>
#include "log/logger.hpp"
#include "registry_impl.hpp"

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

  PrometheusHandler::PrometheusHandler()
      : logger_{log::createLogger("PrometheusHandler", "metrics")} {}

  void PrometheusHandler::onSessionRequest(Session::Request request,
                                           std::shared_ptr<Session> session) {
    std::vector<MetricFamily> metrics;

    {
      std::lock_guard<std::mutex> lock{collectables_mutex_};
      metrics = CollectMetrics(collectables_);
    }

    const TextSerializer serializer;

    auto size = writeResponse(session, request, serializer.Serialize(metrics));
  }

  std::size_t PrometheusHandler::writeResponse(std::shared_ptr<Session> session,
                                               Session::Request request,
                                               const std::string &body) {
    Session::Response res{boost::beast::http::status::ok, request.version()};
    res.set(boost::beast::http::field::content_type,
            "text/plain; charset=utf-8");
    res.content_length(body.size());
    res.keep_alive(true);
    res.body() = body;
    session->respond(res);
    return body.size();
  }

  // it is called once on init
  void PrometheusHandler::registerCollectable(Registry &registry) {
    auto *pregistry = dynamic_cast<PrometheusRegistry *>(&registry);
    if (pregistry) {
      registerCollectable(pregistry->registry());
    }
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
