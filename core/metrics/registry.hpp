#ifndef KAGOME_CORE_METRICS_REGISTRY_HPP
#define KAGOME_CORE_METRICS_REGISTRY_HPP

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace kagome::metrics {

  class Counter;
  class Gauge;
  class Handler;
  class Histogram;
  class Summary;

  class Registry {
   public:
    virtual void setHandler(Handler *handler) = 0;

    virtual void registerCounterFamily(
        const std::string &name,
        const std::string &help = "",
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual void registerGaugeFamily(
        const std::string &name,
        const std::string &help = "",
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual void registerHistogramFamily(
        const std::string &name,
        const std::string &help = "",
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual void registerSummaryFamily(
        const std::string &name,
        const std::string &help = "",
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual Counter *registerCounterMetric(
        const std::string &name,
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual Gauge *registerGaugeMetric(
        const std::string &name,
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual Histogram *registerHistogramMetric(
        const std::string &name,
        const std::vector<double> &bucket_boundaries,
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual Summary *registerSummaryMetric(
        const std::string &name,
        const std::vector<std::pair<double, double>> &quantiles,
        std::chrono::milliseconds max_age = std::chrono::seconds{60},
        int age_buckets = 5,
        const std::map<std::string, std::string> &labels = {}) = 0;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_REGISTRY_HPP
